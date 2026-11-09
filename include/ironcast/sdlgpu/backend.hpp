#pragma once

#include <memory>

#include <SDL3/SDL_gpu.h>

#include "ironcast/gpubackend.hpp"

namespace ironcast::sdlgpu {
    // SDL_GPU settings of a Backend; set through Ironcast::set_backend_factory(), shared by every window it makes.
    struct BackendConfig {
        SDL_GPUShaderFormat shaderFormat = SDL_GPU_SHADERFORMAT_SPIRV;

        // Driver SDL should use ("vulkan", "direct3d12", "metal"); empty lets SDL pick.
        std::string driver;
        // Turns on the driver's validation layers (SDL_CreateGPUDevice's debug_mode).
        bool debug = false;
        // IMMEDIATE matches opengl::Backend's swap interval 0, but not every driver supports it; VSYNC always works.
        SDL_GPUPresentMode presentMode = SDL_GPU_PRESENTMODE_IMMEDIATE;
        // Passed to ImGui_ImplSDLGPU3_InitInfo::MSAASamples.
        SDL_GPUSampleCount msaaSamples = SDL_GPU_SAMPLECOUNT_1;
    };

    // SDL_GPU backend (Vulkan, Metal or D3D12, chosen by SDL): an SDL GPU device and ImGui context per window.
    class Backend: public GPUBackend {
        BackendConfig gpuConfig_;
        std::shared_ptr<SDL_GPUDevice> device_;
        // This window's copy of background_.
        SDL_GPUTexture* backgroundImage_ = nullptr;

    protected:
        ImTextureID upload_background() override;

    public:
        Backend(const WindowConfig &config = {}, const BackendConfig &gpuConfig = {});
        void init() override;
        void destroy() override;
        void update(Widget *rootWidget) override;
        // Shared by every window this backend type makes; nullptr before init().
        [[nodiscard]] SDL_GPUDevice* device() const;
        // A new R8G8B8A8_UNORM texture with SAMPLER usage holding image's pixels, or nullptr on failure. The
        // upload is submitted before this returns. Release it with SDL_ReleaseGPUTexture(), or hand it to a
        // RenderWidget.
        [[nodiscard]] SDL_GPUTexture* upload_texture(const Image& image);
    };
}
