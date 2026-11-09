#include "ironcast/webgpu/backend.hpp"

#include <algorithm>
#include <format>
#include <limits>
#include <span>
#include <string_view>

#include <imgui_impl_sdl3.h>
#include <imgui_impl_wgpu.h>
#include <sdl3webgpu.h>

#include "ironcast/window.hpp"
#include "ironcast/log.hpp"
#include "ironcast/assert.hpp"

namespace ironcast::webgpu {
    struct SharedDevice {
        wgpu::Instance instance;
        wgpu::Adapter adapter;
        wgpu::Device device;
    };

    namespace {
        std::weak_ptr<SharedDevice> sharedDevice;
    }

    Backend::Backend(
        const WindowConfig &config,
        const BackendConfig &gpuConfig
    ) : GPUBackend(config), gpuConfig_(gpuConfig) {}

    void Backend::init() {
        shared_ = sharedDevice.lock();
        if (!shared_) {
            shared_ = std::make_shared<SharedDevice>();
            const wgpu::InstanceFeatureName timedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
            wgpu::InstanceDescriptor instanceDescriptor;
            instanceDescriptor.requiredFeatureCount = 1;
            instanceDescriptor.requiredFeatures = &timedWaitAny;
            shared_->instance = wgpu::CreateInstance(&instanceDescriptor);
            if (!shared_->instance) {
                fail("wgpuCreateInstance failed");
            }
            sharedDevice = shared_;
        }
        const wgpu::Instance& instance = shared_->instance;

        create_window(0);   // WebGPU needs no window flag

        surface_ = wgpu::Surface::Acquire(SDL_GetWGPUSurface(instance.Get(), handle_));
        if (!surface_) {
            fail("SDL_GetWGPUSurface failed");
        }

        // The first window picks the adapter, one that can present to its surface.
        if (!shared_->device) {
            wgpu::RequestAdapterOptions adapterOptions;
            adapterOptions.powerPreference = gpuConfig_.powerPreference;
            adapterOptions.backendType = gpuConfig_.backendType;
            adapterOptions.compatibleSurface = surface_;
            instance.WaitAny(
                instance.RequestAdapter(
                    &adapterOptions,
                    wgpu::CallbackMode::WaitAnyOnly,
                    [this](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message) {
                        if (status != wgpu::RequestAdapterStatus::Success) {
                            fail(std::format("RequestAdapter failed: {}", std::string_view(message)));
                        }
                        shared_->adapter = std::move(adapter);
                    }
                ),
                std::numeric_limits<uint64_t>::max()
            );

            wgpu::DeviceDescriptor deviceDescriptor;
            deviceDescriptor.SetUncapturedErrorCallback(
                [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message) {
                    logger().error("WebGPU error {}: {}", static_cast<int>(type), std::string_view(message));
                }
            );
            instance.WaitAny(
                shared_->adapter.RequestDevice(
                    &deviceDescriptor,
                    wgpu::CallbackMode::WaitAnyOnly,
                    [this](wgpu::RequestDeviceStatus status, wgpu::Device device, wgpu::StringView message) {
                        if (status != wgpu::RequestDeviceStatus::Success) {
                            fail(std::format("RequestDevice failed: {}", std::string_view(message)));
                        }
                        shared_->device = std::move(device);
                    }
                ),
                std::numeric_limits<uint64_t>::max()
            );
        }

        wgpu::SurfaceCapabilities capabilities;
        if (surface_.GetCapabilities(shared_->adapter, &capabilities) != wgpu::Status::Success || capabilities.formatCount == 0) {
            fail("Surface::GetCapabilities failed");
        }

        wgpu::PresentMode presentMode = gpuConfig_.presentMode;
        const auto presentModes = std::span(capabilities.presentModes, capabilities.presentModeCount);
        if (std::ranges::find(presentModes, presentMode) == presentModes.end()) {
            logger().warn("window {}: present mode {} not supported, using Fifo", window_->id(), static_cast<int>(presentMode));
            presentMode = wgpu::PresentMode::Fifo;
        }

        int width = 0, height = 0;
        SDL_GetWindowSizeInPixels(handle_, &width, &height);
        surfaceConfig_.device = shared_->device;
        surfaceConfig_.format = capabilities.formats[0];
        surfaceConfig_.width = static_cast<uint32_t>(width);
        surfaceConfig_.height = static_cast<uint32_t>(height);
        surfaceConfig_.presentMode = presentMode;
        surface_.Configure(&surfaceConfig_);

        create_context();
        if (!ImGui_ImplSDL3_InitForOther(handle_)) {
            fail("ImGui_ImplSDL3_InitForOther failed");
        }

        ImGui_ImplWGPU_InitInfo info;
        info.Device = shared_->device.Get();
        info.RenderTargetFormat = static_cast<WGPUTextureFormat>(surfaceConfig_.format);
        info.PipelineMultisampleState.count = gpuConfig_.msaaSamples;
        if (!ImGui_ImplWGPU_Init(&info)) {
            fail("ImGui_ImplWGPU_Init failed");
        }

        wgpu::AdapterInfo adapterInfo;
        shared_->adapter.GetInfo(&adapterInfo);
        logger().info(
           "window {} '{}' created {}x{}, WebGPU on {}, scale {}",
           window_->id(),
           config_.title,
           config_.width,
           config_.height,
           std::string_view(adapterInfo.description),
           scale_
       );
    }

    void Backend::update(Widget *rootWidget) {
        make_current();
        ImGui_ImplWGPU_NewFrame();
        build_frame(rootWidget);
        ImDrawData* drawData = ImGui::GetDrawData();

        int width = 0, height = 0;
        SDL_GetWindowSizeInPixels(handle_, &width, &height);
        if (width <= 0 || height <= 0) {
            return;                        // minimized: configuring a 0x0 surface is a validation error
        }

        if (static_cast<uint32_t>(width) != surfaceConfig_.width || static_cast<uint32_t>(height) != surfaceConfig_.height) {
            surfaceConfig_.width = width;
            surfaceConfig_.height = height;
            surface_.Configure(&surfaceConfig_);
        }

        wgpu::SurfaceTexture surfaceTexture;
        surface_.GetCurrentTexture(&surfaceTexture);
        if (surfaceTexture.status == wgpu::SurfaceGetCurrentTextureStatus::Error) {
            logger().error("window {}: GetCurrentTexture failed", window_->id());
            return;
        }

        if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal &&
            surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal) {
            // Timeout, Outdated or Lost: reconfigure and skip this frame. Presenting without a texture is invalid.
            surface_.Configure(&surfaceConfig_);
            return;
        }

        wgpu::RenderPassColorAttachment color;
        color.view = surfaceTexture.texture.CreateView();
        color.loadOp = wgpu::LoadOp::Clear;
        color.storeOp = wgpu::StoreOp::Store;
        color.clearValue = {0.0, 0.0, 0.0, 0.0};

        wgpu::RenderPassDescriptor passDescriptor;
        passDescriptor.colorAttachmentCount = 1;
        passDescriptor.colorAttachments = &color;

        wgpu::CommandEncoder encoder = shared_->device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDescriptor);
        ImGui_ImplWGPU_RenderDrawData(drawData, pass.Get());
        pass.End();

        wgpu::CommandBuffer commands = encoder.Finish();
        shared_->device.GetQueue().Submit(1, &commands);

#ifndef __EMSCRIPTEN__
        surface_.Present();
#endif

        shared_->instance.ProcessEvents();
    }

    void Backend::destroy() {
        IRONCAST_DEBUG_ASSERT(handle_ != nullptr, "webgpu::Backend::destroy called with no window handle");
        IRONCAST_DEBUG_ASSERT(shared_ != nullptr, "webgpu::Backend::destroy called with no device");

        logger().info("window {} '{}' closed", window_->id(), config_.title);

        // No wait for idle: WebGPU keeps objects alive until the GPU work using them is done.
        // The backends and the context are per window; shut them down while this window's context is current.
        make_current();
        ImGui_ImplWGPU_Shutdown();
        backgroundView_ = nullptr;
        backgroundImage_ = nullptr;
        destroy_context();

        // Before the window: the surface refers to its native window.
        surface_.Unconfigure();
        surface_ = nullptr;
        SDL_DestroyWindow(handle_);

        // Last: the device goes with the last window that holds it. surfaceConfig_ holds a reference too.
        surfaceConfig_.device = nullptr;
        shared_.reset();
    }

    ImTextureID Backend::upload_background() {
        // Frames in flight keep the old texture alive until they are done with it.
        backgroundView_ = nullptr;
        backgroundImage_ = nullptr;

        if (background_ == nullptr) {
            return ImTextureID_Invalid;
        }

        backgroundImage_ = upload_texture(*background_);
        backgroundView_ = backgroundImage_.CreateView();

        // ImGui's WGPU backend takes a WGPUTextureView as ImTextureID.
        return static_cast<ImTextureID>(reinterpret_cast<intptr_t>(backgroundView_.Get()));
    }

    wgpu::Texture Backend::upload_texture(const Image& image) {
        IRONCAST_DEBUG_ASSERT(shared_ != nullptr, "webgpu::Backend::upload_texture called before init");

        wgpu::TextureDescriptor textureDescriptor;
        textureDescriptor.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
        textureDescriptor.dimension = wgpu::TextureDimension::e2D;
        textureDescriptor.size = {static_cast<uint32_t>(image.width()), static_cast<uint32_t>(image.height())};
        textureDescriptor.format = wgpu::TextureFormat::RGBA8Unorm;
        // Never null: on failure WebGPU returns an invalid texture and reports to the uncaptured-error callback.
        wgpu::Texture texture = shared_->device.CreateTexture(&textureDescriptor);

        // WriteTexture has no row alignment requirement, unlike a buffer-to-texture copy.
        wgpu::TexelCopyTextureInfo destination;
        destination.texture = texture;
        wgpu::TexelCopyBufferLayout layout;
        layout.bytesPerRow = textureDescriptor.size.width * 4;
        layout.rowsPerImage = textureDescriptor.size.height;
        const auto pixels = image.pixels();
        shared_->device.GetQueue().WriteTexture(&destination, pixels.data(), pixels.size(), &layout, &textureDescriptor.size);
        return texture;
    }
}
