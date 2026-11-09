#pragma once

#include <memory>
#include <optional>

#include <vulkan/vulkan_core.h>
#include <function2/function2.hpp>

#include "ironcast/widget.hpp"

namespace ironcast::vulkan {
    // The widget's images, views and command buffers.
    struct RenderTarget;

    // Draws a Vulkan image it owns at this widget's Yoga box; it has no intrinsic size.
    class RenderWidget : public Widget {
    public:
        // Called from show() inside dynamic rendering (vkCmdBeginRendering) on image_view(), with the depth image
        // if depth(), in a command buffer of its own that is submitted right after, on the window's queue and
        // before its frame. The viewport and scissor are set to the whole image, for pipelines that make them
        // dynamic state. Pipelines render with VkPipelineRenderingCreateInfo: format(), and depth_format() if
        // depth(). Copies can't run inside rendering: upload data in your own command buffer first.
        using RenderCallback = fu2::unique_function<void(RenderWidget &self, VkCommandBuffer commandBuffer)>;

    private:
        std::unique_ptr<RenderTarget> target_;
        VkFormat format_ = VK_FORMAT_R8G8B8A8_UNORM;
        RenderCallback onRender_;
        std::optional<ImVec4> clearColor_;
        int textureWidth_ = 0;
        int textureHeight_ = 0;
        bool autoSize_ = false;
        bool hasDepth_ = false;
        bool captureMouse_ = false;
        bool hovered_ = false;
        bool active_ = false;

        // Waits for the GPU, then frees the images (and ImGui's descriptor set for the colour image).
        void release_images();
        void release_depth();

    protected:
        // Auto-sizes the image and runs the render callback; show() calls it before drawing the image.
        void render_texture();

    public:
        RenderWidget(Widget* parent = nullptr);
        ~RenderWidget() override;

        // Replaces the image with a new one of width x height pixels in format, with COLOR_ATTACHMENT and SAMPLED
        // usage; 0 x 0 removes it. Its contents are undefined until the first render. Needs the widget to be in a
        // window. Waits for the GPU to be idle, as frames in flight may still use the old image.
        RenderWidget& set_texture(int width, int height, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
        // Recreates the image at the new size and the same format; the contents are lost. The depth image is
        // resized with it. Does nothing without an image.
        RenderWidget& resize(int width, int height);
        // VK_NULL_HANDLE without an image. Read them after render_texture(): auto-size replaces the image.
        [[nodiscard]] VkImage image() const;
        [[nodiscard]] VkImageView image_view() const;
        [[nodiscard]] VkFormat format() const;
        // ImGui's descriptor set for the image, as ImGui::Image takes it. Vulkan's origin is top-left like
        // ImGui's, so no UV flip is needed.
        [[nodiscard]] ImTextureID texture_id() const;
        // The image's size in pixels, 0 without an image.
        [[nodiscard]] int texture_width() const;
        [[nodiscard]] int texture_height() const;

        // Called every frame the widget is drawn; it also keeps an OnDemand or Adaptive loop drawing.
        RenderWidget& set_on_render(RenderCallback callback);

        // Cleared to before each render callback, with the depth image (to 1) if depth(). Empty by default: the
        // previous contents are kept.
        [[nodiscard]] std::optional<ImVec4> clear_color() const;
        RenderWidget& set_clear_color(std::optional<ImVec4> color);

        // Resizes the image to the Yoga box in pixels (points times the framebuffer scale) whenever the
        // box changes. Needs an image; its initial size does not matter.
        [[nodiscard]] bool auto_size() const;
        RenderWidget& set_auto_size(bool enabled);

        // A depth image in the rendering, sized with the colour image: D32_SFLOAT, or X8_D24_UNORM_PACK32 where
        // that is not supported. Created on the next render. Off by default.
        [[nodiscard]] bool depth() const;
        // The depth image's format, for the pipeline. Needs the widget to be in a window.
        [[nodiscard]] VkFormat depth_format();
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

        // With a render callback and no image, creates an R8G8B8A8_UNORM image and turns auto_size() on.
        void init() override;
        void destroy() override;
        bool show() override;
    };
}
