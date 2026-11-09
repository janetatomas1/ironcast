#pragma once

#include <optional>

#include <SDL3/SDL_gpu.h>
#include <function2/function2.hpp>

#include "thorin/widget.hpp"

namespace thorin::sdlgpu {
    // Draws an SDL_GPU texture at this widget's Yoga box; it has no intrinsic size.
    // The texture must belong to this window's device (Backend::device()).
    class RenderWidget : public Widget {
    public:
        // Called from show() inside a render pass on texture() (with the depth texture if depth()), in a command
        // buffer of its own that is submitted right after, before the window's frame. The viewport covers the
        // texture. Copy passes can't run inside a render pass: upload data in your own command buffer first.
        using RenderCallback = fu2::unique_function<
            void(RenderWidget &self, SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass)
        >;

    private:
        SDL_GPUTexture* texture_ = nullptr;
        SDL_GPUTexture* depth_ = nullptr;
        SDL_GPUTextureFormat format_ = SDL_GPU_TEXTUREFORMAT_INVALID;
        RenderCallback onRender_;
        std::optional<ImVec4> clearColor_;
        int textureWidth_ = 0;
        int textureHeight_ = 0;
        bool autoSize_ = false;
        bool hasDepth_ = false;
        bool captureMouse_ = false;
        bool hovered_ = false;
        bool active_ = false;

        // The window's device; the widget has to be in a window.
        [[nodiscard]] SDL_GPUDevice* device();
        void release_depth();

    protected:
        // Auto-sizes the texture and runs the render callback; show() calls it before drawing the image.
        void render_texture();

    public:
        RenderWidget(Widget* parent = nullptr);

        [[nodiscard]] SDL_GPUTexture* texture() const;
        // Takes ownership of texture, which is width x height pixels in format; nullptr removes it. To render
        // into it, it needs SDL_GPU_TEXTUREUSAGE_COLOR_TARGET as well as SAMPLER. Replacing a texture needs the
        // widget to be in a window.
        RenderWidget& set_texture(SDL_GPUTexture* texture, int width, int height, SDL_GPUTextureFormat format);
        // Recreates texture() at the new size and the same format, with SAMPLER and COLOR_TARGET usage; the
        // contents are lost. The depth texture is resized with it. Does nothing without a texture.
        RenderWidget& resize(int width, int height);
        // texture() as ImGui::Image takes it. SDL_GPU's origin is top-left like ImGui's, so no UV flip is needed.
        // Read it after render_texture(): auto-size replaces the texture.
        [[nodiscard]] ImTextureID texture_id() const;
        // The texture's size in pixels, 0 without a texture.
        [[nodiscard]] int texture_width() const;
        [[nodiscard]] int texture_height() const;

        // Called every frame the widget is drawn; it also keeps an OnDemand or Adaptive loop drawing.
        RenderWidget& set_on_render(RenderCallback callback);

        // Cleared to before each render callback, with the depth texture (to 1) if depth(). Empty by default: the
        // previous contents are kept.
        [[nodiscard]] std::optional<ImVec4> clear_color() const;
        RenderWidget& set_clear_color(std::optional<ImVec4> color);

        // Resizes the texture to the Yoga box in pixels (points times the framebuffer scale) whenever the
        // box changes. Needs a texture; its initial size does not matter.
        [[nodiscard]] bool auto_size() const;
        RenderWidget& set_auto_size(bool enabled);

        // A depth texture in the render pass, sized with the texture: D24_UNORM, or D32_FLOAT where that is not
        // supported. Created on the next render. Off by default.
        [[nodiscard]] bool depth() const;
        // The depth texture's format, for the pipeline's depth_stencil_format. Needs the widget to be in a window.
        [[nodiscard]] SDL_GPUTextureFormat depth_format();
        RenderWidget& set_depth(bool enabled);

        // Puts an invisible button over the box, so mouse drags and the wheel belong to the view instead of
        // moving or scrolling the window around it. Read the input from ImGui::GetIO() in the render callback,
        // with hovered() / active() telling whether it is meant for this view. Off by default.
        [[nodiscard]] bool capture_mouse() const;
        RenderWidget& set_capture_mouse(bool enabled);
        // With capture_mouse(), as of this frame: the mouse is over the view, and a button pressed over it is
        // still held (true while dragging outside the box too). Always false without it.
        [[nodiscard]] bool hovered() const;
        [[nodiscard]] bool active() const;

        // With a render callback and no texture, creates an R8G8B8A8_UNORM texture and turns auto_size() on.
        void init() override;
        void destroy() override;
        bool show() override;
    };
}
