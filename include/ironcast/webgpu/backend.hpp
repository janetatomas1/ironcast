#pragma once

#include <cstdint>
#include <memory>

#include <webgpu/webgpu_cpp.h>

#include "ironcast/gpubackend.hpp"

namespace ironcast::webgpu {
    // WebGPU settings of a Backend; set through Ironcast::set_backend_factory(), shared by every window it makes.
    struct BackendConfig {
        // API Dawn should use (Vulkan, Metal, D3D12, ...); Undefined lets Dawn pick. Ignored in the browser.
        wgpu::BackendType backendType = wgpu::BackendType::Undefined;
        wgpu::PowerPreference powerPreference = wgpu::PowerPreference::HighPerformance;
        // Immediate matches opengl::Backend's swap interval 0, but not every surface supports it; Fifo always works.
        wgpu::PresentMode presentMode = wgpu::PresentMode::Immediate;
        // Passed to ImGui_ImplWGPU_InitInfo::PipelineMultisampleState.count.
        uint32_t msaaSamples = 1;
    };

    // Instance, adapter and device, shared by every window this backend type makes.
    struct SharedDevice;

    // WebGPU backend (Dawn natively, the browser's WebGPU on Emscripten): a surface and ImGui context per window.
    class Backend: public GPUBackend {
        BackendConfig gpuConfig_;
        std::shared_ptr<SharedDevice> shared_;
        wgpu::Surface surface_;
        wgpu::SurfaceConfiguration surfaceConfig_;
        // This window's copy of background_, and the view ImGui draws it through.
        wgpu::Texture backgroundImage_;
        wgpu::TextureView backgroundView_;

    public:
        Backend(const WindowConfig &config = {}, const BackendConfig &gpuConfig = {});
        void init() override;
        void update(Widget *rootWidget) override;
        void destroy() override;
        ImTextureID upload_background() override;
        // A new RGBA8Unorm texture with TextureBinding usage holding image's pixels. The upload is queued before
        // this returns; failures go to the uncaptured-error log.
        [[nodiscard]] wgpu::Texture upload_texture(const Image& image);
    };
}
