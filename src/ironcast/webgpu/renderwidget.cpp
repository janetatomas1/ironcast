#include <cmath>
#include <cstdint>

#include "ironcast/webgpu/renderwidget.hpp"
#include "ironcast/webgpu/backend.hpp"
#include "ironcast/ironcast.hpp"
#include "ironcast/window.hpp"
#include "ironcast/assert.hpp"

namespace ironcast::webgpu {
    namespace {
        wgpu::Texture create_texture(const wgpu::Device& device, int width, int height, wgpu::TextureFormat format, wgpu::TextureUsage usage) {
            wgpu::TextureDescriptor descriptor;
            descriptor.usage = usage;
            descriptor.dimension = wgpu::TextureDimension::e2D;
            descriptor.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
            descriptor.format = format;
            // Never null: on failure WebGPU returns an invalid texture and reports to the uncaptured-error callback.
            return device.CreateTexture(&descriptor);
        }
    }

    RenderWidget::RenderWidget(Widget* parent): Widget("", parent) {}

    const wgpu::Device& RenderWidget::device() {
        IRONCAST_DEBUG_ASSERT(window() != nullptr, "webgpu::RenderWidget needs to be in a window");
        auto* backend = dynamic_cast<Backend*>(window()->backend());
        IRONCAST_DEBUG_ASSERT(backend != nullptr, "webgpu::RenderWidget needs a webgpu::Backend window");
        return backend->device();
    }

    const wgpu::Texture& RenderWidget::texture() const {
        return texture_;
    }

    RenderWidget& RenderWidget::set_texture(wgpu::Texture texture) {
        // Frames in flight keep the old textures alive until they are done with them.
        texture_ = std::move(texture);
        view_ = texture_ ? texture_.CreateView() : nullptr;
        depth_ = nullptr;
        depthView_ = nullptr;
        textureWidth_ = texture_ ? static_cast<int>(texture_.GetWidth()) : 0;
        textureHeight_ = texture_ ? static_cast<int>(texture_.GetHeight()) : 0;
        return *this;
    }

    RenderWidget& RenderWidget::resize(int width, int height) {
        if (!texture_) {
            return *this;
        }
        // The depth texture is recreated at the new size on the next render.
        return set_texture(create_texture(
            device(),
            width,
            height,
            texture_.GetFormat(),
            wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment
        ));
    }

    const wgpu::TextureView& RenderWidget::texture_view() const {
        return view_;
    }

    ImTextureID RenderWidget::texture_id() const {
        // ImGui's WGPU backend takes a WGPUTextureView as ImTextureID.
        return static_cast<ImTextureID>(reinterpret_cast<intptr_t>(view_.Get()));
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

    RenderWidget& RenderWidget::set_depth(bool enabled) {
        hasDepth_ = enabled;
        if (!enabled) {
            depth_ = nullptr;
            depthView_ = nullptr;
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
        if (!texture_ && onRender_) {
            // 1x1 until then: the box is empty before the first layout, and a zero-sized texture is invalid.
            set_texture(create_texture(
                device(),
                1,
                1,
                wgpu::TextureFormat::RGBA8Unorm,
                wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment
            ));
            autoSize_ = true;
        }
        Widget::init();
    }

    void RenderWidget::destroy() {
        // Released before the backend drops the device.
        set_texture(nullptr);
        Widget::destroy();
    }

    void RenderWidget::render_texture() {
        if (!texture_) {
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

        const wgpu::Device& gpu = device();
        if (hasDepth_ && !depth_) {
            depth_ = create_texture(gpu, textureWidth_, textureHeight_, depth_format(), wgpu::TextureUsage::RenderAttachment);
            depthView_ = depth_.CreateView();
        }

        wgpu::RenderPassColorAttachment color;
        color.view = view_;
        color.loadOp = clearColor_ ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
        color.storeOp = wgpu::StoreOp::Store;
        if (clearColor_) {
            color.clearValue = {clearColor_->x, clearColor_->y, clearColor_->z, clearColor_->w};
        }

        wgpu::RenderPassDepthStencilAttachment depth;
        depth.view = depthView_;
        depth.depthLoadOp = clearColor_ ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
        depth.depthStoreOp = wgpu::StoreOp::Store;
        depth.depthClearValue = 1.0f;

        wgpu::RenderPassDescriptor passDescriptor;
        passDescriptor.colorAttachmentCount = 1;
        passDescriptor.colorAttachments = &color;
        passDescriptor.depthStencilAttachment = depthView_ ? &depth : nullptr;

        wgpu::CommandEncoder encoder = gpu.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDescriptor);
        onRender_(*this, encoder, pass);
        pass.End();
        // Submitted before the window's frame, which samples the texture.
        wgpu::CommandBuffer commands = encoder.Finish();
        gpu.GetQueue().Submit(1, &commands);

        Ironcast::current().request_redraw();
    }

    bool RenderWidget::show() {
        if (!texture_) {
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
