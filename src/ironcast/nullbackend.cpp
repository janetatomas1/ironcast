#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>
#include "ironcast/assert.hpp"

#include "ironcast/nullbackend.hpp"

#include "ironcast/window.hpp"
#include "ironcast/log.hpp"

namespace ironcast {
    NullBackend::NullBackend(
        const WindowConfig &config
    ) : GPUBackend(config) {}

    void NullBackend::init() {
        IRONCAST_DEBUG_ASSERT(window_ != nullptr, "NullBackend::init called before set_window");

        create_window(0);
        create_context();

        if (!ImGui_ImplSDL3_InitForOther(handle_)) {
            fail("ImGui_ImplSDL3_InitForOther failed");
        }

        // Like ImGui's null renderer: the atlas builds on demand, and update() acknowledges texture
        // requests without uploading anything.
        ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_RendererHasTextures | ImGuiBackendFlags_RendererHasVtxOffset;

        logger().info(
            "window {} '{}' created {}x{}, null backend, scale {}",
            window_->id(),
            config_.title,
            config_.width,
            config_.height,
            scale_
        );
    }

    void NullBackend::destroy() {
        IRONCAST_DEBUG_ASSERT(handle_ != nullptr, "NullBackend::destroy called with no window handle");

        logger().info("window {} '{}' closed", window_->id(), config_.title);

        make_current();
        destroy_context();
        SDL_DestroyWindow(handle_);
    }

    void NullBackend::update(Widget *rootWidget) {
        make_current();
        build_frame(rootWidget);

        if (auto textures = ImGui::GetDrawData()->Textures; textures != nullptr) {
            for (ImTextureData* texture : *textures) {
                if (texture->Status == ImTextureStatus_WantCreate || texture->Status == ImTextureStatus_WantUpdates) {
                    texture->SetStatus(ImTextureStatus_OK);
                } else if (texture->Status == ImTextureStatus_WantDestroy) {
                    texture->SetStatus(ImTextureStatus_Destroyed);
                }
            }
        }
    }
}
