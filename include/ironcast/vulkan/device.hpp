#pragma once

// Internals shared by ironcast_vulkan's sources: the shared device, the frames in flight and the barrier helper.
// Include it only to extend the Vulkan backend (e.g. a widget of your own). It pulls in vulkan_raii.hpp, VMA
// and vk-bootstrap, and may change between minor versions.

#include <cstdint>
#include <limits>

#include <VkBootstrap.h>
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>

namespace ironcast::vulkan {
    inline constexpr uint32_t apiVersion = VK_API_VERSION_1_3;
    inline constexpr uint32_t framesInFlight = 2;
    inline constexpr uint64_t noTimeout = std::numeric_limits<uint64_t>::max();
    // RenderWidgets with a texture per window: each holds one of ImGui's descriptor sets.
    inline constexpr uint32_t maxRenderWidgets = 64;

    struct SharedDevice {
        vk::raii::Context context;
        // Kept for vkb::PhysicalDeviceSelector and vkb::SwapchainBuilder; they own nothing.
        vkb::Instance vkbInstance;
        vkb::Device vkbDevice;
        vk::raii::Instance instance = nullptr;
        vk::raii::DebugUtilsMessengerEXT messenger = nullptr;
        vk::raii::PhysicalDevice physicalDevice = nullptr;
        vk::raii::Device device = nullptr;
        uint32_t queueFamily = 0;
        vk::raii::Queue queue = nullptr;
        VmaAllocator allocator = nullptr;

        explicit SharedDevice(PFN_vkGetInstanceProcAddr getInstanceProcAddr) : context(getInstanceProcAddr) {}

        ~SharedDevice() {
            // Before the device; the members go after this, in reverse order.
            if (allocator != nullptr) {
                vmaDestroyAllocator(allocator);
            }
        }
    };

    inline void barrier(
        const vk::raii::CommandBuffer& commandBuffer,
        vk::Image image,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::PipelineStageFlags2 srcStage,
        vk::AccessFlags2 srcAccess,
        vk::PipelineStageFlags2 dstStage,
        vk::AccessFlags2 dstAccess,
        vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor
    ) {
        vk::ImageMemoryBarrier2 imageBarrier;
        imageBarrier
            .setSrcStageMask(srcStage)
            .setSrcAccessMask(srcAccess)
            .setDstStageMask(dstStage)
            .setDstAccessMask(dstAccess)
            .setOldLayout(oldLayout)
            .setNewLayout(newLayout)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(image)
            .setSubresourceRange({aspect, 0, 1, 0, 1});
        commandBuffer.pipelineBarrier2(vk::DependencyInfo().setImageMemoryBarriers(imageBarrier));
    }
}
