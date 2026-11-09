
#include <cstdlib>
#include <format>

#include <imgui_impl_sdl3.h>
#include "ironcast/assert.hpp"

#include "ironcast/gpubackend.hpp"

#include "ironcast/ironcast.hpp"
#include "ironcast/window.hpp"

namespace ironcast {
    GPUBackend::GPUBackend(
        const WindowConfig &config
    ): window_(nullptr), config_(config) {}

    void GPUBackend::set_window(Window* window) {
        window_ = window;
    }

    void GPUBackend::fail(const std::string &message) {
        logger().critical(message);
        spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& l) { l->flush(); });
        std::abort();
    }

    void GPUBackend::create_window(SDL_WindowFlags apiFlags) {
        IRONCAST_DEBUG_ASSERT(window_ != nullptr, "GPUBackend::create_window called before set_window");

        scale_ = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        if (scale_ <= 0.0f) {
            // SDL reports 0 when it cannot query the display; a zero scale would collapse the whole UI.
            scale_ = 1.0f;
        }

        handle_ = SDL_CreateWindow(
            config_.title.c_str(),
            config_.width,
            config_.height,
            config_.sdlFlags | apiFlags
        );
        if (handle_ == nullptr) {
            fail(std::format("could not create window '{}': {}", config_.title, SDL_GetError()));
        }

        if (config_.windowState == WindowState::MAXIMIZED) {
            maximize();
        } else if (config_.windowState == WindowState::MINIMIZED) {
            minimize();
        }

        // WindowManager finds the Window of an SDL event through it.
        SDL_SetPointerProperty(SDL_GetWindowProperties(handle_), "WRAPPER", window_);
        SDL_SetWindowPosition(handle_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_ShowWindow(handle_);
    }

    void GPUBackend::create_context() {
        IMGUI_CHECKVERSION();
        imguiContext_ = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        // ImGui keeps the pointer, not a copy; config_ outlives the context.
        io.IniFilename = config_.iniFile.empty() ? nullptr : config_.iniFile.c_str();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        apply_theme(window_->app().theme());
    }

    void GPUBackend::destroy_context() {
        IRONCAST_DEBUG_ASSERT(imguiContext_ != nullptr, "GPUBackend::destroy_context called with no imgui context");

        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext(imguiContext_);
        imguiContext_ = nullptr;
    }

    void GPUBackend::make_current() {
        IRONCAST_DEBUG_ASSERT(imguiContext_ != nullptr, "make_current called with no imgui context");

        ImGui::SetCurrentContext(imguiContext_);
    }

    void GPUBackend::build_frame(Widget *rootWidget) {
        IRONCAST_DEBUG_ASSERT(handle_ != nullptr, "GPUBackend::build_frame called with no window handle");
        IRONCAST_DEBUG_ASSERT(rootWidget != nullptr, "GPUBackend::build_frame called with null root widget");

        SDL_GetWindowSize(handle_, &config_.width, &config_.height);

        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(config_.width, config_.height));
        // Yoga padding is the only inset: the root box covers the whole window from (0, 0), so ImGui's padding
        // would only push the content past the edge and bring up a scrollbar.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
        // A fixed name: every ImGui ID below derives from it, so renaming the window keeps their state.
        ImGui::Begin(
            "##ironcast_root",
            nullptr,
            config_.windowFlags
        );
        ImGui::PopStyleVar();
        draw_background();

        rootWidget->layout().calculate_layout(config_.width, config_.height);
        rootWidget->render();

        ImGui::End();
        window_->update_inspector();
        ImGui::Render();
    }

    ImTextureID GPUBackend::upload_background() {
        return ImTextureID_Invalid;
    }

    void GPUBackend::draw_background() {
        // The root window's own draw list: above its WindowBg, below every widget.
        const ImVec2 pos = ImGui::GetWindowPos();
        const ImVec2 size = ImGui::GetWindowSize();
        const ImVec2 max(pos.x + size.x, pos.y + size.y);

        if (backgroundTexture_ != ImTextureID_Invalid) {
            ImGui::GetWindowDrawList()->AddImage(
                backgroundTexture_,
                pos,
                max,
                ImVec2(0, 0),
                ImVec2(1, 1),
                ImGui::GetColorU32(ImVec4(1, 1, 1, backgroundOpacity_))
            );
        }

        if (backgroundPainter_) {
            backgroundPainter_(*ImGui::GetWindowDrawList(), pos, max);
        }
    }

    void GPUBackend::process_event(const SDL_Event& event) {
        IRONCAST_DEBUG_ASSERT(imguiContext_ != nullptr, "process_event called with no imgui context");

        ImGui::SetCurrentContext(imguiContext_);
        ImGui_ImplSDL3_ProcessEvent(&event);
    }

    void GPUBackend::apply_theme(const Theme &theme) {
        // Always from the unscaled theme: scaling an already scaled style would compound.
        ImGuiStyle& style = ImGui::GetStyle();
        style = theme.style;

        if (style.FontSizeBase <= 0.0f) {
            style.FontSizeBase = defaultFontSize;
        }
        style.ScaleAllSizes(scale_);
        style.FontScaleDpi = scale_;

        if (theme.font != font_) {
            ImFontAtlas* atlas = ImGui::GetIO().Fonts;

            if (imFont_ != nullptr) {
                atlas->RemoveFont(imFont_);
                imFont_ = nullptr;
            }

            if (theme.font != nullptr) {
                ImFontConfig config;
                // Borrowed: the Font outlives this window, the atlas must not free its bytes.
                config.FontDataOwnedByAtlas = false;
                // ImGui never writes to the data; the parameter just isn't const.
                imFont_ = atlas->AddFontFromMemoryTTF(
                    const_cast<std::byte*>(theme.font->data().data()),
                    static_cast<int>(theme.font->data().size()),
                    0.0f,
                    &config
                );
            }
            font_ = theme.font;
            ImGui::GetIO().FontDefault = imFont_;
        }
    }

    const Image* GPUBackend::background() const {
        return background_;
    }

    void GPUBackend::set_background(const Image* image, float opacity) {
        // Only a new image needs a new texture; opacity is applied when drawing.
        const bool changed = image != background_;
        background_ = image;
        backgroundOpacity_ = opacity;

        // Before init() there is no context to upload into; Window::init() uploads it.
        if (changed && handle_ != nullptr) {
            window_->post([](Window& window) {
                window.backend()->update_background();
            });
        }
    }

    void GPUBackend::update_background() {
        make_current();
        backgroundTexture_ = upload_background();
    }

    void GPUBackend::set_background_painter(BackgroundPainter painter) {
        backgroundPainter_ = std::move(painter);
    }

    bool GPUBackend::has_background_painter() const {
        return static_cast<bool>(backgroundPainter_);
    }

    const Image* GPUBackend::icon() const {
        return icon_;
    }

    void GPUBackend::set_icon(const Image* image) {
        icon_ = image;

        // SDL can't take an icon away; nullptr only forgets it.
        if (handle_ == nullptr || image == nullptr) {
            return;
        }
        // Image pixels are RGBA8 in byte order; SDL only reads them while copying into the window's icon.
        SDL_Surface* surface = SDL_CreateSurfaceFrom(
            image->width(),
            image->height(),
            SDL_PIXELFORMAT_RGBA32,
            const_cast<std::byte*>(image->pixels().data()),
            image->width() * 4
        );

        if (surface == nullptr || !SDL_SetWindowIcon(handle_, surface)) {
            logger().warn("window {} '{}': can't set icon: {}", window_->id(), config_.title, SDL_GetError());
        }
        SDL_DestroySurface(surface);
    }

    SDL_Window* GPUBackend::handle() const {
        return handle_;
    }

    Window* GPUBackend::window() {
        return window_;
    }

    void GPUBackend::maximize() {
        window_->post([](Window& window) {
            SDL_MaximizeWindow(window.backend()->handle_);
        });
    }

    void GPUBackend::minimize() {
        window_->post([](Window& window) {
            SDL_MinimizeWindow(window.backend()->handle_);
        });
    }

    void GPUBackend::set_size(int width, int height) {
        window_->post([width, height](Window& window) {
            SDL_SetWindowSize(window.backend()->handle_, width, height);
        });
    }

    void GPUBackend::set_title(const std::string& title) {
        window_->post([title](Window& window) {
            GPUBackend* backend = window.backend();
            backend->config_.title = title;
            SDL_SetWindowTitle(backend->handle_, title.c_str());
        });
    }

    const std::string& GPUBackend::title() const {
        return config_.title;
    }

    int GPUBackend::height() const {
        return config_.height;
    }

    int GPUBackend::width() const {
        return config_.width;
    }
}
