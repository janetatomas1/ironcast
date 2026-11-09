#include <glbinding/gl/gl.h>
#include <globjects/globjects.h>

#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include "ironcast/assert.hpp"

#include "ironcast/opengl/backend.hpp"

#include <format>

#include "ironcast/window.hpp"
#include "ironcast/ironcast.hpp"
#include "ironcast/log.hpp"


using namespace gl;

namespace ironcast::opengl {
    namespace {
        // The #version line for a context: core 3.2 -> "150 core", from 3.3 on it matches (4.1 -> "410 core");
        // ES matches too (3.0 -> "300 es").
        std::string glsl_version(const BackendConfig &config) {
            const int major = config.majorVersion;
            const int minor = config.minorVersion;

            if (config.es) {
                IRONCAST_DEBUG_ASSERT(major >= 3, "GLES needs 3.0 or later", major, minor);
                return std::format("#version {} es", major * 100 + minor * 10);
            }
            IRONCAST_DEBUG_ASSERT(major > 3 || (major == 3 && minor >= 2), "core profile needs GL 3.2 or later", major, minor);
            return std::format("#version {} core", major == 3 && minor == 2 ? 150 : major * 100 + minor * 10);
        }
    }

    Backend::Backend(
        const WindowConfig &config,
        const BackendConfig &glConfig
    ) : GPUBackend(config), glContext_(nullptr), glConfig_(glConfig) {}

    void Backend::init() {
        IRONCAST_DEBUG_ASSERT(window_ != nullptr, "opengl::Backend::init called before set_window");

#ifdef __EMSCRIPTEN__
        // SDL would still create a WebGL context, and the core-profile shaders would fail to compile.
        if (!glConfig_.es) {
            fail("opengl::BackendConfig::es must be true on Emscripten: browsers only provide GLES (WebGL2)");
        }
#endif

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, glConfig_.majorVersion);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, glConfig_.minorVersion);
        SDL_GL_SetAttribute(
            SDL_GL_CONTEXT_PROFILE_MASK,
            glConfig_.es ? SDL_GL_CONTEXT_PROFILE_ES : SDL_GL_CONTEXT_PROFILE_CORE
        );
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, glConfig_.depthBits);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, glConfig_.stencilBits);

        create_window(SDL_WINDOW_OPENGL);

        glContext_ = SDL_GL_CreateContext(handle_);
        if (glContext_ == nullptr) {
            fail(std::format(
                "could not create an OpenGL {} {}.{} context: {}",
                glConfig_.es ? "ES" : "core",
                glConfig_.majorVersion,
                glConfig_.minorVersion,
                SDL_GetError()
            ));
        }

        if (!SDL_GL_MakeCurrent(handle_, glContext_)) {
            fail(std::format("SDL_GL_MakeCurrent failed: {}", SDL_GetError()));
        }

        // No vsync: the frame policy paces the loop (DisplayMatched for the display's rate). Vsync would
        // stack one refresh wait per window.
        SDL_GL_SetSwapInterval(0);

        // Also initializes glbinding for this context.
        globjects::init(
            [](const char* name) {
                return SDL_GL_GetProcAddress(name);
            }
        );

        create_context();

        if (!ImGui_ImplSDL3_InitForOpenGL(handle_, glContext_)) {
            fail("ImGui_ImplSDL3_InitForOpenGL failed");
        }

        // ImGui copies the string.
        const std::string glslVersion = glsl_version(glConfig_);
        if (!ImGui_ImplOpenGL3_Init(glslVersion.c_str())) {
            fail(std::format("ImGui_ImplOpenGL3_Init failed with '{}'", glslVersion));
        }

        logger().info(
            "window {} '{}' created {}x{}, OpenGL {} on {}, scale {}",
            window_->id(),
            config_.title,
            config_.width,
            config_.height,
            reinterpret_cast<const char*>(glGetString(GL_VERSION)),
            reinterpret_cast<const char*>(glGetString(GL_RENDERER)),
            scale_
        );
    }

    void Backend::destroy() {
        IRONCAST_DEBUG_ASSERT(handle_ != nullptr, "opengl::Backend::destroy called with no window handle");
        IRONCAST_DEBUG_ASSERT(glContext_ != nullptr, "opengl::Backend::destroy called with no gl context");

        logger().info("window {} '{}' closed", window_->id(), config_.title);

        // The backends and the context are per window; shut them down while this window's contexts are current.
        make_current();
        // GL objects have to go while their context is current.
        backgroundImage_.reset();
        ImGui_ImplOpenGL3_Shutdown();
        destroy_context();

        SDL_GL_DestroyContext(glContext_);
        SDL_DestroyWindow(handle_);
    }

    void Backend::update(Widget *rootWidget) {
        make_current();

        ImGui_ImplOpenGL3_NewFrame();
        build_frame(rootWidget);

        // config_ holds the logical size; on a high-density display the framebuffer has more pixels.
        int pixelWidth = 0;
        int pixelHeight = 0;
        SDL_GetWindowSizeInPixels(handle_, &pixelWidth, &pixelHeight);
        glViewport(0, 0, pixelWidth, pixelHeight);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(handle_);
    }

    ImTextureID Backend::upload_background() {
        backgroundImage_.reset();

        if (background_ == nullptr) {
            return ImTextureID_Invalid;
        }
        backgroundImage_ = globjects::Texture::createDefault(GL_TEXTURE_2D);
        backgroundImage_->image2D(
            0,
            GL_RGBA8,
            background_->width(),
            background_->height(),
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            background_->pixels().data()
        );
        return static_cast<ImTextureID>(backgroundImage_->id());
    }

    void Backend::make_current() {
        IRONCAST_DEBUG_ASSERT(handle_ != nullptr, "make_current called with no window handle");
        IRONCAST_DEBUG_ASSERT(glContext_ != nullptr, "make_current called with no gl context");

        SDL_GL_MakeCurrent(handle_, glContext_);
        // Each window registers its own context in globjects::init.
        globjects::setCurrentContext();
        GPUBackend::make_current();
    }
}
