#include "ironcast/vulkan/backend.hpp"
#include "ironcast/vulkan/device.hpp"

#include <array>
#include <cstdint>
#include <format>
#include <string_view>
#include <vector>

#include <SDL3/SDL_vulkan.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include "ironcast/window.hpp"
#include "ironcast/log.hpp"
#include "ironcast/assert.hpp"

namespace ironcast::vulkan {
    namespace {
        std::weak_ptr<SharedDevice> sharedDevice;

        struct Frame {
            vk::raii::CommandPool commandPool = nullptr;
            vk::raii::CommandBuffer commandBuffer = nullptr;
            // Signaled when the frame's commands are done, so its command buffer and semaphore can be reused.
            vk::raii::Fence fence = nullptr;
            vk::raii::Semaphore imageAcquired = nullptr;
        };
    }

    // Destroyed in reverse order: the background, frames and views before the swapchain, the swapchain before the surface.
    struct WindowState {
        vk::raii::SurfaceKHR surface = nullptr;
        vk::raii::SwapchainKHR swapchain = nullptr;
        VkFormat format = VK_FORMAT_UNDEFINED;
        vk::Extent2D extent;
        std::vector<vk::Image> images;
        std::vector<vk::raii::ImageView> views;
        // One per swapchain image: a semaphore can be signaled again only once the present waiting on it is done,
        // which getting the same image back from acquire guarantees.
        std::vector<vk::raii::Semaphore> renderFinished;
        std::array<Frame, framesInFlight> frames;
        uint32_t frameIndex = 0;
        // Acquire or present reported that the swapchain no longer matches the surface.
        bool swapchainDirty = false;
        // background_ as uploaded, and ImGui's descriptor set for it.
        VkImage backgroundImage = VK_NULL_HANDLE;
        VmaAllocation backgroundAllocation = nullptr;
        vk::raii::ImageView backgroundView = nullptr;
        VkDescriptorSet backgroundSet = VK_NULL_HANDLE;
    };

    namespace {
        VKAPI_ATTR VkBool32 VKAPI_CALL log_validation(
            VkDebugUtilsMessageSeverityFlagBitsEXT severity,
            VkDebugUtilsMessageTypeFlagsEXT,
            const VkDebugUtilsMessengerCallbackDataEXT* data,
            void*
        ) {
            if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
                logger().error("Vulkan: {}", data->pMessage);
            } else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
                logger().warn("Vulkan: {}", data->pMessage);
            } else {
                logger().debug("Vulkan: {}", data->pMessage);
            }
            return VK_FALSE;
        }

        void check_imgui_result(VkResult result) {
            if (result < 0) {
                logger().error("ImGui Vulkan backend: VkResult {}", static_cast<int>(result));
            }
        }

        // Needs ImGui's Vulkan backend, and the GPU to be done with the image.
        void release_background(const SharedDevice& shared, WindowState& state) {
            if (state.backgroundSet != VK_NULL_HANDLE) {
                ImGui_ImplVulkan_RemoveTexture(state.backgroundSet);
                state.backgroundSet = VK_NULL_HANDLE;
            }
            state.backgroundView = nullptr;
            if (state.backgroundImage != VK_NULL_HANDLE) {
                vmaDestroyImage(shared.allocator, state.backgroundImage, state.backgroundAllocation);
                state.backgroundImage = VK_NULL_HANDLE;
                state.backgroundAllocation = nullptr;
            }
        }
    }

    Backend::Backend(
        const WindowConfig &config,
        const BackendConfig &gpuConfig
    ) : GPUBackend(config), gpuConfig_(gpuConfig) {}

    Backend::~Backend() = default;

    void Backend::init() try {
        // SDL loads the Vulkan library with the first SDL_WINDOW_VULKAN window.
        create_window(SDL_WINDOW_VULKAN);
        state_ = std::make_unique<WindowState>();

        shared_ = sharedDevice.lock();
        if (!shared_) {
            const auto getInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
            if (getInstanceProcAddr == nullptr) {
                fail(std::format("SDL_Vulkan_GetVkGetInstanceProcAddr failed: {}", SDL_GetError()));
            }
            shared_ = std::make_shared<SharedDevice>(getInstanceProcAddr);

            Uint32 extensionCount = 0;
            const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
            vkb::InstanceBuilder instanceBuilder(getInstanceProcAddr);
            instanceBuilder
                .set_app_name(config_.title.c_str())
                .require_api_version(apiVersion)
                .enable_extensions(extensionCount, extensions);

            if (gpuConfig_.debug) {
                instanceBuilder.request_validation_layers().set_debug_callback(log_validation);
            }

            auto vkbInstance = instanceBuilder.build();
            if (!vkbInstance) {
                fail(std::format("Vulkan instance creation failed: {}", vkbInstance.error().message()));
            }

            shared_->vkbInstance = *vkbInstance;
            shared_->instance = vk::raii::Instance(shared_->context, vkbInstance->instance);
            if (vkbInstance->debug_messenger != VK_NULL_HANDLE) {
                shared_->messenger = vk::raii::DebugUtilsMessengerEXT(shared_->instance, vkbInstance->debug_messenger);
            }
            sharedDevice = shared_;
        }

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if (!SDL_Vulkan_CreateSurface(handle_, static_cast<VkInstance>(*shared_->instance), nullptr, &surface)) {
            fail(std::format("SDL_Vulkan_CreateSurface failed: {}", SDL_GetError()));
        }
        state_->surface = vk::raii::SurfaceKHR(shared_->instance, surface);

        // The first window picks the GPU, one that can present to its surface.
        if (shared_->allocator == nullptr) {
            VkPhysicalDeviceVulkan13Features features13{};
            features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
            features13.synchronization2 = VK_TRUE;
            features13.dynamicRendering = VK_TRUE;
            auto physical = vkb::PhysicalDeviceSelector(shared_->vkbInstance, surface)
                .set_minimum_version(1, 3)
                .set_required_features_13(features13)
                .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
                .select();
            if (!physical) {
                fail(std::format("no Vulkan 1.3 GPU found: {}", physical.error().message()));
            }
            auto vkbDevice = vkb::DeviceBuilder(*physical).build();
            if (!vkbDevice) {
                fail(std::format("Vulkan device creation failed: {}", vkbDevice.error().message()));
            }
            auto queueFamily = vkbDevice->get_queue_index(vkb::QueueType::graphics);
            if (!queueFamily) {
                fail(std::format("no Vulkan graphics queue: {}", queueFamily.error().message()));
            }
            shared_->vkbDevice = *vkbDevice;
            shared_->physicalDevice = vk::raii::PhysicalDevice(shared_->instance, physical->physical_device);
            shared_->device = vk::raii::Device(shared_->physicalDevice, vkbDevice->device);
            shared_->queueFamily = *queueFamily;
            shared_->queue = vk::raii::Queue(shared_->device, shared_->queueFamily, 0);

            VmaVulkanFunctions functions{};
            functions.vkGetInstanceProcAddr = shared_->context.getDispatcher()->vkGetInstanceProcAddr;
            functions.vkGetDeviceProcAddr = shared_->instance.getDispatcher()->vkGetDeviceProcAddr;
            VmaAllocatorCreateInfo allocatorInfo{};
            allocatorInfo.vulkanApiVersion = apiVersion;
            allocatorInfo.physicalDevice = physical->physical_device;
            allocatorInfo.device = vkbDevice->device;
            allocatorInfo.instance = static_cast<VkInstance>(*shared_->instance);
            allocatorInfo.pVulkanFunctions = &functions;
            if (vmaCreateAllocator(&allocatorInfo, &shared_->allocator) != VK_SUCCESS) {
                fail("vmaCreateAllocator failed");
            }
        }

        // Frames are drawn and presented on the one graphics queue.
        if (!shared_->physicalDevice.getSurfaceSupportKHR(shared_->queueFamily, *state_->surface)) {
            fail(std::format("window {}: the Vulkan graphics queue can't present to it", window_->id()));
        }

        recreate_swapchain();

        for (Frame& frame : state_->frames) {
            frame.commandPool = vk::raii::CommandPool(
                shared_->device,
                vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eTransient, shared_->queueFamily)
            );
            frame.commandBuffer = std::move(vk::raii::CommandBuffers(
                shared_->device,
                vk::CommandBufferAllocateInfo(*frame.commandPool, vk::CommandBufferLevel::ePrimary, 1)
            ).front());
            // Signaled, so the first wait on it returns at once.
            frame.fence = vk::raii::Fence(shared_->device, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
            frame.imageAcquired = vk::raii::Semaphore(shared_->device, vk::SemaphoreCreateInfo());
        }

        create_context();
        if (!ImGui_ImplSDL3_InitForVulkan(handle_)) {
            fail("ImGui_ImplSDL3_InitForVulkan failed");
        }

        // imgui is built with IMGUI_IMPL_VULKAN_NO_PROTOTYPES: it gets its functions from the same loader.
        const bool loaded = ImGui_ImplVulkan_LoadFunctions(
            apiVersion,
            [](const char* name, void* userData) {
                const auto& shared = *static_cast<SharedDevice*>(userData);
                return shared.context.getDispatcher()->vkGetInstanceProcAddr(static_cast<VkInstance>(*shared.instance), name);
            },
            shared_.get()
        );
        if (!loaded) {
            fail("ImGui_ImplVulkan_LoadFunctions failed");
        }

        ImGui_ImplVulkan_InitInfo info{};
        info.ApiVersion = apiVersion;
        info.Instance = static_cast<VkInstance>(*shared_->instance);
        info.PhysicalDevice = static_cast<VkPhysicalDevice>(*shared_->physicalDevice);
        info.Device = static_cast<VkDevice>(*shared_->device);
        info.QueueFamily = shared_->queueFamily;
        info.Queue = static_cast<VkQueue>(*shared_->queue);
        // ImGui creates its own pool: the font atlas, the background, and one per RenderWidget.
        info.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE + 1 + maxRenderWidgets;
        info.MinImageCount = 2;
        info.ImageCount = static_cast<uint32_t>(state_->images.size());
        info.UseDynamicRendering = true;
        info.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &state_->format;
        info.CheckVkResultFn = check_imgui_result;
        if (!ImGui_ImplVulkan_Init(&info)) {
            fail("ImGui_ImplVulkan_Init failed");
        }

        logger().info(
           "window {} '{}' created {}x{}, Vulkan on {}, scale {}",
           window_->id(),
           config_.title,
           config_.width,
           config_.height,
           std::string_view(shared_->physicalDevice.getProperties().deviceName.data()),
           scale_
       );
    } catch (const vk::SystemError& error) {
        fail(std::format("Vulkan setup failed: {}", error.what()));
    }

    void Backend::recreate_swapchain() {
        const vk::raii::Device& device = shared_->device;
        // The old swapchain's images, views and semaphores may still be in use.
        device.waitIdle();

        int width = 0, height = 0;
        SDL_GetWindowSizeInPixels(handle_, &width, &height);

        auto swapchain = vkb::SwapchainBuilder(shared_->vkbDevice, static_cast<VkSurfaceKHR>(*state_->surface))
            // UNORM: ImGui's colours are already sRGB-encoded, an SRGB format would encode them again.
            .set_desired_format({VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
            .add_fallback_format({VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
            .set_desired_present_mode(gpuConfig_.presentMode)
            .add_fallback_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(static_cast<uint32_t>(width), static_cast<uint32_t>(height))
            .set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
            .set_old_swapchain(static_cast<VkSwapchainKHR>(*state_->swapchain))
            .build();
        if (!swapchain) {
            fail(std::format("window {}: swapchain creation failed: {}", window_->id(), swapchain.error().message()));
        }
        if (!*state_->swapchain && swapchain->present_mode != gpuConfig_.presentMode) {
            logger().warn("window {}: present mode {} not supported, using FIFO", window_->id(), static_cast<int>(gpuConfig_.presentMode));
        }

        // The views and semaphores of the old images go first, then the old swapchain.
        state_->views.clear();
        state_->renderFinished.clear();
        state_->swapchain = vk::raii::SwapchainKHR(device, swapchain->swapchain);
        state_->format = swapchain->image_format;
        state_->extent = swapchain->extent;
        state_->images = state_->swapchain.getImages();
        for (vk::Image image : state_->images) {
            state_->views.emplace_back(device, vk::ImageViewCreateInfo(
                {},
                image,
                vk::ImageViewType::e2D,
                static_cast<vk::Format>(state_->format),
                {},
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
            ));
            state_->renderFinished.emplace_back(device, vk::SemaphoreCreateInfo());
        }
        state_->swapchainDirty = false;
    }

    void Backend::update(Widget *rootWidget) try {
        make_current();
        ImGui_ImplVulkan_NewFrame();
        build_frame(rootWidget);
        ImDrawData* drawData = ImGui::GetDrawData();

        int width = 0, height = 0;
        SDL_GetWindowSizeInPixels(handle_, &width, &height);
        if (width <= 0 || height <= 0) {
            return;                        // minimized: a 0x0 swapchain can't be created
        }
        if (state_->swapchainDirty
            || static_cast<uint32_t>(width) != state_->extent.width
            || static_cast<uint32_t>(height) != state_->extent.height) {
            recreate_swapchain();
        }

        Frame& frame = state_->frames[state_->frameIndex];
        (void)shared_->device.waitForFences(*frame.fence, VK_TRUE, noTimeout);

        auto [acquired, imageIndex] = state_->swapchain.acquireNextImage(noTimeout, *frame.imageAcquired);
        if (acquired == vk::Result::eErrorOutOfDateKHR) {
            // No image and nothing signaled: recreate next frame and skip this one.
            state_->swapchainDirty = true;
            return;
        }
        if (acquired == vk::Result::eSuboptimalKHR) {
            state_->swapchainDirty = true;   // still usable for this frame
        }
        // Only now: with the early return above, a reset fence would never be signaled and the next wait would hang.
        shared_->device.resetFences(*frame.fence);

        frame.commandPool.reset();
        const vk::raii::CommandBuffer& commandBuffer = frame.commandBuffer;
        commandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        const vk::Image image = state_->images[imageIndex];
        // Undefined: the image is cleared, its old contents don't matter. Ordered after the acquire semaphore,
        // which the submit waits on at the same stage.
        barrier(
            commandBuffer, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite
        );

        vk::RenderingAttachmentInfo color;
        color
            .setImageView(*state_->views[imageIndex])
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue(vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}));
        vk::RenderingInfo rendering;
        rendering
            .setRenderArea({{0, 0}, state_->extent})
            .setLayerCount(1)
            .setColorAttachments(color);
        commandBuffer.beginRendering(rendering);
        ImGui_ImplVulkan_RenderDrawData(drawData, static_cast<VkCommandBuffer>(*commandBuffer));
        commandBuffer.endRendering();

        barrier(
            commandBuffer, image, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone
        );
        commandBuffer.end();

        const vk::SemaphoreSubmitInfo waitInfo(*frame.imageAcquired, 0, vk::PipelineStageFlagBits2::eColorAttachmentOutput);
        const vk::SemaphoreSubmitInfo signalInfo(*state_->renderFinished[imageIndex], 0, vk::PipelineStageFlagBits2::eAllGraphics);
        const vk::CommandBufferSubmitInfo commandBufferInfo(*commandBuffer);
        vk::SubmitInfo2 submit;
        submit
            .setWaitSemaphoreInfos(waitInfo)
            .setCommandBufferInfos(commandBufferInfo)
            .setSignalSemaphoreInfos(signalInfo);
        shared_->queue.submit2(submit, *frame.fence);

        const vk::Semaphore renderFinished = *state_->renderFinished[imageIndex];
        const vk::SwapchainKHR swapchain = *state_->swapchain;
        vk::PresentInfoKHR present;
        present
            .setWaitSemaphores(renderFinished)
            .setSwapchains(swapchain)
            .setImageIndices(imageIndex);
        const vk::Result presented = shared_->queue.presentKHR(present);
        if (presented == vk::Result::eErrorOutOfDateKHR || presented == vk::Result::eSuboptimalKHR) {
            state_->swapchainDirty = true;
        }

        state_->frameIndex = (state_->frameIndex + 1) % framesInFlight;
    } catch (const vk::SystemError& error) {
        fail(std::format("window {}: Vulkan frame failed: {}", window_->id(), error.what()));
    }

    void Backend::destroy() try {
        IRONCAST_DEBUG_ASSERT(handle_ != nullptr, "vulkan::Backend::destroy called with no window handle");
        IRONCAST_DEBUG_ASSERT(shared_ != nullptr, "vulkan::Backend::destroy called with no device");

        logger().info("window {} '{}' closed", window_->id(), config_.title);

        // Frames in flight may still use the swapchain images, semaphores and the background.
        shared_->device.waitIdle();

        // The backends and the context are per window; shut them down while this window's context is current.
        make_current();
        release_background(*shared_, *state_);
        ImGui_ImplVulkan_Shutdown();
        destroy_context();

        // Before the window: the swapchain and surface refer to it.
        state_.reset();
        // The device goes with the last window that holds it. Also before the window: SDL unloads the Vulkan
        // library with the last Vulkan window, and the device and instance are destroyed through it.
        shared_.reset();
        SDL_DestroyWindow(handle_);
    } catch (const vk::SystemError& error) {
        fail(std::format("window {}: Vulkan teardown failed: {}", window_->id(), error.what()));
    }

    SharedDevice& Backend::shared_device() const {
        IRONCAST_DEBUG_ASSERT(shared_ != nullptr, "vulkan::Backend::shared_device called before init");
        return *shared_;
    }

    ImTextureID Backend::upload_background() try {
        const vk::raii::Device& device = shared_->device;
        // The GPU may still be drawing the old image. Replacing the background is rare, so wait rather than defer.
        device.waitIdle();
        release_background(*shared_, *state_);

        if (background_ == nullptr) {
            return ImTextureID_Invalid;
        }

        const auto width = static_cast<uint32_t>(background_->width());
        const auto height = static_cast<uint32_t>(background_->height());
        const auto pixels = background_->pixels();

        const VkImageCreateInfo imageInfo = vk::ImageCreateInfo(
            {},
            vk::ImageType::e2D,
            vk::Format::eR8G8B8A8Unorm,
            {width, height, 1},
            1,
            1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
        );
        VmaAllocationCreateInfo imageAllocationInfo{};
        imageAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
        if (vmaCreateImage(shared_->allocator, &imageInfo, &imageAllocationInfo, &state_->backgroundImage, &state_->backgroundAllocation, nullptr) != VK_SUCCESS) {
            logger().error("window {}: vmaCreateImage failed", window_->id());
            return ImTextureID_Invalid;
        }
        const vk::Image image(state_->backgroundImage);

        const VkBufferCreateInfo stagingInfo = vk::BufferCreateInfo({}, pixels.size(), vk::BufferUsageFlagBits::eTransferSrc);
        VmaAllocationCreateInfo stagingAllocationInfo{};
        stagingAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        VkBuffer staging = VK_NULL_HANDLE;
        VmaAllocation stagingAllocation = nullptr;
        if (vmaCreateBuffer(shared_->allocator, &stagingInfo, &stagingAllocationInfo, &staging, &stagingAllocation, nullptr) != VK_SUCCESS) {
            logger().error("window {}: vmaCreateBuffer failed", window_->id());
            release_background(*shared_, *state_);
            return ImTextureID_Invalid;
        }
        // Maps, copies and flushes.
        vmaCopyMemoryToAllocation(shared_->allocator, pixels.data(), stagingAllocation, 0, pixels.size());

        vk::raii::CommandPool commandPool(device, vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eTransient, shared_->queueFamily));
        vk::raii::CommandBuffer commandBuffer = std::move(vk::raii::CommandBuffers(
            device,
            vk::CommandBufferAllocateInfo(*commandPool, vk::CommandBufferLevel::ePrimary, 1)
        ).front());
        commandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        barrier(
            commandBuffer, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
            vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferWrite
        );
        const vk::BufferImageCopy region(0, 0, 0, {vk::ImageAspectFlagBits::eColor, 0, 0, 1}, {0, 0, 0}, {width, height, 1});
        commandBuffer.copyBufferToImage(vk::Buffer(staging), image, vk::ImageLayout::eTransferDstOptimal, region);
        barrier(
            commandBuffer, image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferWrite,
            vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderSampledRead
        );
        commandBuffer.end();

        const vk::raii::Fence fence(device, vk::FenceCreateInfo());
        const vk::CommandBufferSubmitInfo commandBufferInfo(*commandBuffer);
        shared_->queue.submit2(vk::SubmitInfo2().setCommandBufferInfos(commandBufferInfo), *fence);
        (void)device.waitForFences(*fence, VK_TRUE, noTimeout);
        vmaDestroyBuffer(shared_->allocator, staging, stagingAllocation);

        state_->backgroundView = vk::raii::ImageView(device, vk::ImageViewCreateInfo(
            {},
            image,
            vk::ImageViewType::e2D,
            vk::Format::eR8G8B8A8Unorm,
            {},
            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
        ));
        state_->backgroundSet = ImGui_ImplVulkan_AddTexture(
            static_cast<VkImageView>(*state_->backgroundView),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
        // ImGui's Vulkan backend takes a VkDescriptorSet as ImTextureID.
        return static_cast<ImTextureID>(reinterpret_cast<intptr_t>(state_->backgroundSet));
    } catch (const vk::SystemError& error) {
        fail(std::format("window {}: background upload failed: {}", window_->id(), error.what()));
    }
}
