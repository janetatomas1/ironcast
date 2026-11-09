#include <cmath>
#include <cstdint>

#include "ironcast/sdlgpu/renderwidget.hpp"
#include "ironcast/sdlgpu/backend.hpp"
#include "ironcast/ironcast.hpp"
#include "ironcast/window.hpp"
#include "ironcast/log.hpp"
#include "ironcast/assert.hpp"

namespace ironcast::sdlgpu {
    RenderWidget::RenderWidget(Widget* parent): Widget("", parent) {}

    SDL_GPUDevice* RenderWidget::device() {
        IRONCAST_DEBUG_ASSERT(window() != nullptr, "sdlgpu::RenderWidget needs to be in a window");
        auto* backend = dynamic_cast<Backend*>(window()->backend());
        IRONCAST_DEBUG_ASSERT(backend != nullptr, "sdlgpu::RenderWidget needs an sdlgpu::Backend window");
        return backend->device();
    }

    void RenderWidget::release_depth() {
        if (depth_ != nullptr) {
            SDL_ReleaseGPUTexture(device(), depth_);
            depth_ = nullptr;
        }
    }

    SDL_GPUTexture* RenderWidget::texture() const {
        return texture_;
    }

    RenderWidget& RenderWidget::set_texture(
        SDL_GPUTexture* texture,
        int width,
        int height,
        SDL_GPUTextureFormat format
    ) {
        // SDL defers the release until frames in flight are done with it.
        if (texture_ != nullptr && texture_ != texture) {
            SDL_ReleaseGPUTexture(device(), texture_);
        }
        release_depth();

        texture_ = texture;
        textureWidth_ = texture_ != nullptr ? width : 0;
        textureHeight_ = texture_ != nullptr ? height : 0;
        format_ = format;
        return *this;
    }

    RenderWidget& RenderWidget::resize(int width, int height) {
        if (texture_ == nullptr) {
            return *this;
        }

        SDL_GPUTextureCreateInfo info{};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = format_;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        info.width = static_cast<Uint32>(width);
        info.height = static_cast<Uint32>(height);
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        SDL_GPUTexture* texture = SDL_CreateGPUTexture(device(), &info);
        if (texture == nullptr) {
            logger().error("sdlgpu::RenderWidget: SDL_CreateGPUTexture failed: {}", SDL_GetError());
            return *this;
        }
        // The depth texture is recreated at the new size on the next render.
        return set_texture(texture, width, height, format_);
    }

    ImTextureID RenderWidget::texture_id() const {
        // ImGui_ImplSDLGPU3 takes the SDL_GPUTexture* as the id.
        return static_cast<ImTextureID>(reinterpret_cast<intptr_t>(texture_));
    }

    int RenderWidget::texture_width() const {
        return textureWidth_;
    }

    int RenderWidget::texture_height() const {
        return textureHeight_;
    }

    RenderWidget& RenderWidget::set_on_render(RenderCallback callback) {
        onRender_ = std::move(callback);
        return *this;
    }

    std::optional<ImVec4> RenderWidget::clear_color() const {
        return clearColor_;
    }

    RenderWidget& RenderWidget::set_clear_color(std::optional<ImVec4> color) {
        clearColor_ = color;
        return *this;
    }

    bool RenderWidget::auto_size() const {
        return autoSize_;
    }

    RenderWidget& RenderWidget::set_auto_size(bool enabled) {
        autoSize_ = enabled;
        return *this;
    }

    bool RenderWidget::depth() const {
        return hasDepth_;
    }

    SDL_GPUTextureFormat RenderWidget::depth_format() {
        // Every device supports at least one of the two.
        const bool d24 = SDL_GPUTextureSupportsFormat(
            device(),
            SDL_GPU_TEXTUREFORMAT_D24_UNORM,
            SDL_GPU_TEXTURETYPE_2D,
            SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET
        );
        return d24 ? SDL_GPU_TEXTUREFORMAT_D24_UNORM : SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    }

    RenderWidget& RenderWidget::set_depth(bool enabled) {
        hasDepth_ = enabled;
        if (!enabled) {
            release_depth();
        }
        return *this;
    }

    bool RenderWidget::capture_mouse() const {
        return captureMouse_;
    }

    RenderWidget& RenderWidget::set_capture_mouse(bool enabled) {
        captureMouse_ = enabled;
        hovered_ = false;
        active_ = false;
        return *this;
    }

    bool RenderWidget::hovered() const {
        return hovered_;
    }

    bool RenderWidget::active() const {
        return active_;
    }

    void RenderWidget::init() {
        // A render callback with no texture gets a default one, sized to the box from the first render on.
        if (texture_ == nullptr && onRender_) {
            SDL_GPUTextureCreateInfo info{};
            info.type = SDL_GPU_TEXTURETYPE_2D;
            info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
            info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
            // 1x1 until then: the box is empty before the first layout, and a zero-sized texture is invalid.
            info.width = 1;
            info.height = 1;
            info.layer_count_or_depth = 1;
            info.num_levels = 1;
            SDL_GPUTexture* texture = SDL_CreateGPUTexture(device(), &info);
            if (texture == nullptr) {
                logger().error("sdlgpu::RenderWidget: SDL_CreateGPUTexture failed: {}", SDL_GetError());
            } else {
                set_texture(texture, 1, 1, info.format);
                autoSize_ = true;
            }
        }
        Widget::init();
    }

    void RenderWidget::destroy() {
        // Released before the backend destroys the device.
        if (texture_ != nullptr) {
            set_texture(nullptr, 0, 0, format_);
        }
        Widget::destroy();
    }

    void RenderWidget::render_texture() {
        if (texture_ == nullptr) {
            return;
        }

        if (autoSize_) {
            // Windows are high pixel density, so the box in points is not the pixel count.
            const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
            const int width = static_cast<int>(std::lround(size().x * scale.x));
            const int height = static_cast<int>(std::lround(size().y * scale.y));

            // The box is empty until the first layout; a zero-sized texture is invalid.
            if (width > 0 && height > 0 && (width != textureWidth_ || height != textureHeight_)) {
                resize(width, height);
            }
        }

        if (!onRender_ || textureWidth_ <= 0 || textureHeight_ <= 0) {
            return;
        }

        SDL_GPUDevice* gpu = device();
        if (hasDepth_ && depth_ == nullptr) {
            SDL_GPUTextureCreateInfo info{};
            info.type = SDL_GPU_TEXTURETYPE_2D;
            info.format = depth_format();
            info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
            info.width = static_cast<Uint32>(textureWidth_);
            info.height = static_cast<Uint32>(textureHeight_);
            info.layer_count_or_depth = 1;
            info.num_levels = 1;
            depth_ = SDL_CreateGPUTexture(gpu, &info);
            if (depth_ == nullptr) {
                logger().error("sdlgpu::RenderWidget: depth SDL_CreateGPUTexture failed: {}", SDL_GetError());
            }
        }

        SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(gpu);
        if (commandBuffer == nullptr) {
            logger().error("sdlgpu::RenderWidget: SDL_AcquireGPUCommandBuffer failed: {}", SDL_GetError());
            return;
        }

        SDL_GPUColorTargetInfo color{};
        color.texture = texture_;
        color.load_op = clearColor_ ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
        color.store_op = SDL_GPU_STOREOP_STORE;
        if (clearColor_) {
            color.clear_color = SDL_FColor{clearColor_->x, clearColor_->y, clearColor_->z, clearColor_->w};
        }

        SDL_GPUDepthStencilTargetInfo depth{};
        depth.texture = depth_;
        depth.load_op = clearColor_ ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
        depth.store_op = SDL_GPU_STOREOP_STORE;
        depth.clear_depth = 1.0f;
        depth.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        depth.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
            commandBuffer,
            &color,
            1,
            depth_ != nullptr ? &depth : nullptr
        );
        onRender_(*this, commandBuffer, renderPass);
        SDL_EndGPURenderPass(renderPass);
        // Submitted before the window's frame, which samples the texture.
        SDL_SubmitGPUCommandBuffer(commandBuffer);

        Ironcast::current().request_redraw();
    }

    bool RenderWidget::show() {
        if (texture_ == nullptr) {
            return false;
        }

        if (!captureMouse_) {
            render_texture();
            ImGui::Image(texture_id(), size());
            return false;
        }

        // The button comes first so the render callback sees this frame's hovered / active state.
        ImGui::InvisibleButton(
            "##view",
            size(),
            ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle
        );
        hovered_ = ImGui::IsItemHovered();
        active_ = ImGui::IsItemActive();
        // Keeps the wheel from scrolling a parent window.
        if (hovered_) {
            ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
        }
        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();

        render_texture();
        ImGui::GetWindowDrawList()->AddImage(texture_id(), min, max);
        return false;
    }
}
