
#pragma once

#include <string>
#include <memory>

#include <SDL3/SDL.h>
#include <globjects/Texture.h>

#include "ironcast/gpubackend.hpp"

namespace ironcast::opengl {
    // OpenGL settings of a Backend; set through Ironcast::set_backend_factory(), shared by every window it makes.
    struct BackendConfig {
#ifdef __EMSCRIPTEN__
        // Browsers only have GLES (WebGL2 is GLES 3.0).
        bool es = true;
        int majorVersion = 3,
        minorVersion = 0;
#else
        // An OpenGL ES context instead of a core-profile one.
        bool es = false;
        // Requested context version: core 3.2 or later, or ES 3.0 or later. The GLSL version is derived from
        // it (core 3.2 -> 150, 4.1 -> 410; ES 3.0 -> 300 es).
        int majorVersion = 3,
        minorVersion = 3;
#endif
        int depthBits = 24,
        stencilBits = 8;
    };

    // OpenGL backend, core profile or GLES (3.3 core by default, GLES 3.0 on Emscripten; see
    // BackendConfig): an SDL GL context and ImGui context per window.
    class Backend: public GPUBackend {
        SDL_GLContext glContext_;
        BackendConfig glConfig_;
        // This window's copy of background_, in its GL context.
        std::unique_ptr<globjects::Texture> backgroundImage_;

    protected:
        ImTextureID upload_background() override;

    public:
        Backend(const WindowConfig &config = {}, const BackendConfig &glConfig = {});
        void init() override;
        void destroy() override;
        void update(Widget *rootWidget) override;
        void make_current() override;
    };
}
