
#pragma once

#include <memory>
#include <string>
#include <SDL3/SDL.h>
#include <function2/function2.hpp>
#include <imgui.h>

#include "ironcast/windowconfig.hpp"
#include "ironcast/theme.hpp"
#include "ironcast/image.hpp"


namespace ironcast {
    class Window;
    class Widget;

    // Draws a window's background each frame into its root draw list: above WindowBg and the background image,
    // below every widget. Called with the window's rectangle.
    using BackgroundPainter = fu2::unique_function<void(ImDrawList& list, ImVec2 min, ImVec2 max)>;

    class GPUBackend;
    // Makes the backend of a new window; see Ironcast::set_backend_factory().
    using BackendFactory = fu2::unique_function<std::unique_ptr<GPUBackend>(const WindowConfig& config)>;

    // The per-window platform and GPU part of a Window: SDL window, GPU and ImGui contexts, theme, fonts,
    // background and icon. Subclasses implement one graphics API (opengl::Backend, sdlgpu::Backend): its context, the ImGui renderer
    // backend, the background texture and presenting; the protected helpers do the rest.
    class GPUBackend {
    protected:
        SDL_Window* handle_ = nullptr;
        ImGuiContext* imguiContext_ = nullptr;
        Window *window_;
        WindowConfig config_;
        // Display content scale the theme's sizes are multiplied by.
        float scale_ = 1.0f;
        // The theme font this window's atlas borrows, to tell when the theme's font changes.
        const Font* font_ = nullptr;
        ImFont* imFont_ = nullptr;
        // Drawn stretched behind the root widget; the backend uploads its own texture of it.
        const Image* background_ = nullptr;
        // 0 = invisible, 1 = opaque; WindowBg shows through the rest.
        float backgroundOpacity_ = 1.0f;
        // background_ as uploaded by upload_background(); ImTextureID_Invalid draws no image.
        ImTextureID backgroundTexture_ = ImTextureID_Invalid;
        BackgroundPainter backgroundPainter_;
        const Image* icon_ = nullptr;

        // Logs message as critical and aborts. For setup failures (window, context, renderer), which have to
        // stop the app even where IRONCAST_DEBUG_ASSERT is compiled out (release builds, Emscripten).
        [[noreturn]] static void fail(const std::string &message);
        // Sets scale_, creates handle_ with config_.sdlFlags | apiFlags (e.g. SDL_WINDOW_OPENGL), applies
        // windowState, registers the window for event routing, then centres and shows it.
        void create_window(SDL_WindowFlags apiFlags);
        // Creates this window's ImGui context, sets up ImGuiIO and applies the app's theme.
        // Call before initialising the ImGui SDL3 and renderer backends.
        void create_context();
        // Shuts down ImGui's SDL3 backend and destroys the context. Call after the renderer backend's shutdown.
        void destroy_context();
        // One frame of UI, after the renderer backend's NewFrame: starts the ImGui frame, lays out and renders
        // the root widget in an ImGui window filling the SDL window, and ends with ImGui::Render(). The
        // subclass then draws ImGui::GetDrawData() and presents.
        void build_frame(Widget *rootWidget);
        // Replaces this backend's texture of background_ (none when it is nullptr) and returns its id, or
        // ImTextureID_Invalid for no image. Called with this window's contexts current. The default has no
        // image support.
        virtual ImTextureID upload_background();

    private:
        // The background image and painter, into the root window's draw list.
        void draw_background();

    public:
        GPUBackend(const WindowConfig &config);
        virtual ~GPUBackend() = default;
        virtual void init() = 0;
        // Draws one frame: the root widget in an ImGui window filling the SDL window.
        virtual void update(Widget *rootWidget) = 0;
        virtual void destroy() = 0;
        // Hands the event to this window's ImGui SDL3 backend.
        virtual void process_event(const SDL_Event& event);
        // Makes this window's GPU and ImGui contexts the current ones. The default sets only the ImGui context;
        // backends with a GPU context switch theirs, then call it.
        virtual void make_current();
        void set_window(Window *window);
        // Copies the theme into the current ImGui context, scaled; call make_current() first.
        void apply_theme(const Theme &theme);
        [[nodiscard]] const Image* background() const;
        // nullptr removes it. The image must stay alive while it is set. A new image is uploaded on the next
        // dispatch, or by Window::init() if the window has not been created yet.
        void set_background(const Image* image, float opacity = 1.0f);
        // Makes the contexts current and uploads background_.
        void update_background();
        // nullptr / empty removes it.
        void set_background_painter(BackgroundPainter painter);
        [[nodiscard]] bool has_background_painter() const;
        [[nodiscard]] const Image* icon() const;
        // Applied now if the window exists, else remembered for Window::init(). SDL copies the pixels.
        void set_icon(const Image* image);
        Window* window();
        [[nodiscard]] SDL_Window* handle() const;
        void maximize();
        void minimize();
        void set_size(int width, int height);
        void set_title(const std::string& title);
        [[nodiscard]] const std::string& title() const;
        [[nodiscard]] int height() const;
        [[nodiscard]] int width() const;
    };
}
