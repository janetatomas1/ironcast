#include "ironcast/vulkan/renderwidget.hpp"
#include "ironcast/vulkan/device.hpp"

#include <array>
#include <cmath>
#include <cstdint>

#include <imgui_impl_vulkan.h>

#include "ironcast/vulkan/backend.hpp"
#include "ironcast/ironcast.hpp"
#include "ironcast/window.hpp"
#include "ironcast/log.hpp"
#include "ironcast/assert.hpp"

namespace ironcast::vulkan {
    namespace {
        struct Slot {
            vk::raii::CommandPool commandPool = nullptr;
            vk::raii::CommandBuffer commandBuffer = nullptr;
            // Signaled when the slot's commands are done, so its command buffer can be reused.
            vk::raii::Fence fence = nullptr;
        };
    }

    // Destroyed in reverse order: views before the slots. The images are freed by release_images().
    struct RenderTarget {
        SharedDevice& shared;
        std::array<Slot, framesInFlight> slots;
        uint32_t slotIndex = 0;

        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = nullptr;
        vk::raii::ImageView view = nullptr;
        // ImGui's descriptor set for view, the widget's ImTextureID.
        VkDescriptorSet set = VK_NULL_HANDLE;
        vk::ImageLayout layout = vk::ImageLayout::eUndefined;

        VkImage depthImage = VK_NULL_HANDLE;
        VmaAllocation depthAllocation = nullptr;
        vk::raii::ImageView depthView = nullptr;
        vk::ImageLayout depthLayout = vk::ImageLayout::eUndefined;

        explicit RenderTarget(SharedDevice& device) : shared(device) {}
    };

    namespace {
        SharedDevice& shared_device(Widget& widget) {
            IRONCAST_DEBUG_ASSERT(widget.window() != nullptr, "vulkan::RenderWidget needs to be in a window");
            auto* backend = dynamic_cast<Backend*>(widget.window()->backend());
            IRONCAST_DEBUG_ASSERT(backend != nullptr, "vulkan::RenderWidget needs a vulkan::Backend window");
            return backend->shared_device();
        }

        // A width x height image with a view of the given aspect, or false on failure.
        bool create_image(
            SharedDevice& shared,
            int width,
            int height,
            VkFormat format,
            vk::ImageUsageFlags usage,
            vk::ImageAspectFlags aspect,
            VkImage& image,
            VmaAllocation& allocation,
            vk::raii::ImageView& view
        ) {
            const VkImageCreateInfo imageInfo = vk::ImageCreateInfo(
                {},
                vk::ImageType::e2D,
                static_cast<vk::Format>(format),
                {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
                1,
                1,
                vk::SampleCountFlagBits::e1,
                vk::ImageTiling::eOptimal,
                usage
            );
            VmaAllocationCreateInfo allocationInfo{};
            allocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
            if (vmaCreateImage(shared.allocator, &imageInfo, &allocationInfo, &image, &allocation, nullptr) != VK_SUCCESS) {
                logger().error("vulkan::RenderWidget: vmaCreateImage failed");
                return false;
            }
            view = vk::raii::ImageView(shared.device, vk::ImageViewCreateInfo(
                {},
                image,
                vk::ImageViewType::e2D,
                static_cast<vk::Format>(format),
                {},
                {aspect, 0, 1, 0, 1}
            ));
            return true;
        }
    }

    RenderWidget::RenderWidget(Widget* parent): Widget("", parent) {}

    RenderWidget::~RenderWidget() = default;

    void RenderWidget::release_depth() {
        if (target_->depthImage != VK_NULL_HANDLE) {
            target_->depthView = nullptr;
            vmaDestroyImage(target_->shared.allocator, target_->depthImage, target_->depthAllocation);
            target_->depthImage = VK_NULL_HANDLE;
            target_->depthAllocation = nullptr;
            target_->depthLayout = vk::ImageLayout::eUndefined;
        }
    }

    void RenderWidget::release_images() {
        // Frames in flight may still render into or sample the images.
        target_->shared.device.waitIdle();
        if (target_->set != VK_NULL_HANDLE) {
            ImGui_ImplVulkan_RemoveTexture(target_->set);
            target_->set = VK_NULL_HANDLE;
        }
        target_->view = nullptr;
        if (target_->image != VK_NULL_HANDLE) {
            vmaDestroyImage(target_->shared.allocator, target_->image, target_->allocation);
            target_->image = VK_NULL_HANDLE;
            target_->allocation = nullptr;
            target_->layout = vk::ImageLayout::eUndefined;
        }
        release_depth();
    }

    RenderWidget& RenderWidget::set_texture(int width, int height, VkFormat format) try {
        format_ = format;
        textureWidth_ = 0;
        textureHeight_ = 0;

        if (target_ == nullptr) {
            if (width <= 0 || height <= 0) {
                return *this;
            }
            SharedDevice& shared = shared_device(*this);
            target_ = std::make_unique<RenderTarget>(shared);
            for (Slot& slot : target_->slots) {
                slot.commandPool = vk::raii::CommandPool(
                    shared.device,
                    vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eTransient, shared.queueFamily)
                );
                slot.commandBuffer = std::move(vk::raii::CommandBuffers(
                    shared.device,
                    vk::CommandBufferAllocateInfo(*slot.commandPool, vk::CommandBufferLevel::ePrimary, 1)
                ).front());
                // Signaled, so the first wait on it returns at once.
                slot.fence = vk::raii::Fence(shared.device, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
            }
        }

        // ImGui's descriptor sets belong to this window's ImGui context.
        window()->backend()->make_current();
        release_images();
        if (width <= 0 || height <= 0) {
            return *this;
        }

        if (!create_image(
            target_->shared,
            width,
            height,
            format,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
            vk::ImageAspectFlagBits::eColor,
            target_->image,
            target_->allocation,
            target_->view
        )) {
            return *this;
        }
        target_->set = ImGui_ImplVulkan_AddTexture(
            static_cast<VkImageView>(*target_->view),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
        textureWidth_ = width;
        textureHeight_ = height;
        return *this;
    } catch (const vk::SystemError& error) {
        logger().error("vulkan::RenderWidget: image creation failed: {}", error.what());
        return *this;
    }

    RenderWidget& RenderWidget::resize(int width, int height) {
        if (image() == VK_NULL_HANDLE) {
            return *this;
        }
        // The depth image is recreated at the new size on the next render.
        return set_texture(width, height, format_);
    }

    VkImage RenderWidget::image() const {
        return target_ != nullptr ? target_->image : VK_NULL_HANDLE;
    }

    VkImageView RenderWidget::image_view() const {
        return target_ != nullptr ? static_cast<VkImageView>(*target_->view) : VK_NULL_HANDLE;
    }

    VkFormat RenderWidget::format() const {
        return format_;
    }

    ImTextureID RenderWidget::texture_id() const {
        if (target_ == nullptr || target_->set == VK_NULL_HANDLE) {
            return ImTextureID_Invalid;
        }
        // ImGui's Vulkan backend takes a VkDescriptorSet as ImTextureID.
        return static_cast<ImTextureID>(reinterpret_cast<intptr_t>(target_->set));
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

    VkFormat RenderWidget::depth_format() {
        // Every device supports at least one of the two as a depth attachment.
        const vk::FormatProperties properties = shared_device(*this).physicalDevice.getFormatProperties(vk::Format::eD32Sfloat);
        const bool d32 = static_cast<bool>(properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment);
        return d32 ? VK_FORMAT_D32_SFLOAT : VK_FORMAT_X8_D24_UNORM_PACK32;
    }

    RenderWidget& RenderWidget::set_depth(bool enabled) {
        hasDepth_ = enabled;
        if (!enabled && target_ != nullptr && target_->depthImage != VK_NULL_HANDLE) {
            // A frame in flight may still render into it.
            target_->shared.device.waitIdle();
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
        // A render callback with no image gets a default one, sized to the box from the first render on.
        if (image() == VK_NULL_HANDLE && onRender_) {
            // 1x1 until then: the box is empty before the first layout, and a zero-sized image is invalid.
            set_texture(1, 1, VK_FORMAT_R8G8B8A8_UNORM);
            autoSize_ = image() != VK_NULL_HANDLE;
        }
        Widget::init();
    }

    void RenderWidget::destroy() {
        // Freed before the backend destroys the device, in this window's ImGui context.
        if (target_ != nullptr) {
            window()->backend()->make_current();
            release_images();
            target_.reset();
        }
        Widget::destroy();
    }

    void RenderWidget::render_texture() try {
        if (image() == VK_NULL_HANDLE) {
            return;
        }

        if (autoSize_) {
            // Windows are high pixel density, so the box in points is not the pixel count.
            const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
            const int width = static_cast<int>(std::lround(size().x * scale.x));
            const int height = static_cast<int>(std::lround(size().y * scale.y));

            // The box is empty until the first layout; a zero-sized image is invalid.
            if (width > 0 && height > 0 && (width != textureWidth_ || height != textureHeight_)) {
                resize(width, height);
            }
        }

        if (!onRender_ || image() == VK_NULL_HANDLE) {
            return;
        }

        RenderTarget& target = *target_;
        const SharedDevice& shared = target.shared;
        if (hasDepth_ && target.depthImage == VK_NULL_HANDLE) {
            create_image(
                target.shared,
                textureWidth_,
                textureHeight_,
                depth_format(),
                vk::ImageUsageFlagBits::eDepthStencilAttachment,
                vk::ImageAspectFlagBits::eDepth,
                target.depthImage,
                target.depthAllocation,
                target.depthView
            );
        }

        Slot& slot = target.slots[target.slotIndex];
        (void)shared.device.waitForFences(*slot.fence, VK_TRUE, noTimeout);
        shared.device.resetFences(*slot.fence);
        slot.commandPool.reset();
        const vk::raii::CommandBuffer& commandBuffer = slot.commandBuffer;
        commandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        // After the previous frame's sampling, which was submitted earlier on the same queue.
        const bool fresh = target.layout == vk::ImageLayout::eUndefined;
        barrier(
            commandBuffer, target.image, target.layout, vk::ImageLayout::eColorAttachmentOptimal,
            fresh ? vk::PipelineStageFlagBits2::eNone : vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite
        );
        if (target.depthImage != VK_NULL_HANDLE) {
            barrier(
                commandBuffer, target.depthImage, target.depthLayout, vk::ImageLayout::eDepthAttachmentOptimal,
                vk::PipelineStageFlagBits2::eLateFragmentTests, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
                vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                vk::ImageAspectFlagBits::eDepth
            );
        }

        const vk::AttachmentLoadOp loadOp = clearColor_ ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
        vk::RenderingAttachmentInfo color;
        color
            .setImageView(*target.view)
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(loadOp)
            .setStoreOp(vk::AttachmentStoreOp::eStore);
        if (clearColor_) {
            color.setClearValue(vk::ClearColorValue(std::array<float, 4>{clearColor_->x, clearColor_->y, clearColor_->z, clearColor_->w}));
        }
        vk::RenderingAttachmentInfo depth;
        depth
            .setImageView(*target.depthView)
            .setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
            .setLoadOp(loadOp)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue(vk::ClearDepthStencilValue(1.0f, 0));

        const vk::Extent2D extent(static_cast<uint32_t>(textureWidth_), static_cast<uint32_t>(textureHeight_));
        vk::RenderingInfo rendering;
        rendering
            .setRenderArea({{0, 0}, extent})
            .setLayerCount(1)
            .setColorAttachments(color);
        if (target.depthImage != VK_NULL_HANDLE) {
            rendering.setPDepthAttachment(&depth);
        }
        commandBuffer.beginRendering(rendering);
        commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f));
        commandBuffer.setScissor(0, vk::Rect2D({0, 0}, extent));
        onRender_(*this, static_cast<VkCommandBuffer>(*commandBuffer));
        commandBuffer.endRendering();

        // For the window's frame, submitted after this on the same queue.
        barrier(
            commandBuffer, target.image, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead
        );
        commandBuffer.end();

        const vk::CommandBufferSubmitInfo commandBufferInfo(*commandBuffer);
        shared.queue.submit2(vk::SubmitInfo2().setCommandBufferInfos(commandBufferInfo), *slot.fence);
        target.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
        if (target.depthImage != VK_NULL_HANDLE) {
            target.depthLayout = vk::ImageLayout::eDepthAttachmentOptimal;
        }
        target.slotIndex = (target.slotIndex + 1) % framesInFlight;

        Ironcast::current().request_redraw();
    } catch (const vk::SystemError& error) {
        logger().error("vulkan::RenderWidget: render failed: {}", error.what());
    }

    bool RenderWidget::show() {
        if (image() == VK_NULL_HANDLE) {
            return false;
        }

        if (!captureMouse_) {
            render_texture();
            // Not drawn until it has been rendered into: its contents are undefined until then.
            if (target_ != nullptr && target_->layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
                ImGui::Image(texture_id(), size());
            } else {
                ImGui::Dummy(size());
            }
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
        if (target_ != nullptr && target_->layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
            ImGui::GetWindowDrawList()->AddImage(texture_id(), min, max);
        }
        return false;
    }
}
