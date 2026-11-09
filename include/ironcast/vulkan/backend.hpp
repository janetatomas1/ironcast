#pragma once

#include <memory>

#include <vulkan/vulkan_core.h>

#include "ironcast/gpubackend.hpp"

namespace ironcast::vulkan {
    // Vulkan settings of a Backend; set through Ironcast::set_backend_factory(), shared by every window it makes.
    struct BackendConfig {
        // Turns on the validation layers (they have to be installed), with their messages in the log.
        bool debug = false;
        // IMMEDIATE matches opengl::Backend's swap interval 0, but not every surface supports it; FIFO always works.
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    };

    // Instance, device, queue and allocator, shared by every window this backend type makes.
    struct SharedDevice;
    // A window's surface, swapchain and frames in flight.
    struct WindowState;

    // Vulkan 1.3 backend (dynamic rendering, synchronization2): a device shared by all windows, and a surface,
    // swapchain and ImGui context per window.
    class Backend: public GPUBackend {
        BackendConfig gpuConfig_;
        std::shared_ptr<SharedDevice> shared_;
        std::unique_ptr<WindowState> state_;

        // Creates the swapchain at the window's size in pixels, replacing the current one.
        void recreate_swapchain();

    protected:
        ImTextureID upload_background() override;

    public:
        Backend(const WindowConfig &config = {}, const BackendConfig &gpuConfig = {});
        ~Backend() override;
        void init() override;
        void update(Widget *rootWidget) override;
        void destroy() override;
        // Shared by every window this backend type makes; for RenderWidget. Call it after init().
        [[nodiscard]] SharedDevice& shared_device() const;
    };
}
