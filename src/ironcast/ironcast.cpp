#include <algorithm>

#include "ironcast/assert.hpp"

#include "ironcast/ironcast.hpp"
#include "ironcast/version.hpp"
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

namespace ironcast {
    RandomGenerator Ironcast::randomGenerator_{};

    void Ironcast::init() {}

    void Ironcast::update() {
        actionManager_.dispatch();
        windowManager_.update();

        frame_ += 1;
        shouldExit_ |= windowManager_.count() == 0;
    }

    Ironcast::Ironcast([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
        IRONCAST_DEBUG_ASSERT(argc >= 0, "argc is negative", argc);
        IRONCAST_DEBUG_ASSERT(argc == 0 || argv != nullptr, "argv is null but argc is nonzero", argc);
        IRONCAST_DEBUG_ASSERT(current_ == nullptr, "only one Ironcast may exist at a time");
        current_ = this;
        configure_logging({});

        // SDL's own messages (errors, warnings) go to the ironcast logger.
        SDL_SetLogOutputFunction(
            [](void*, int category, SDL_LogPriority priority, const char* message) {
                spdlog::level::level_enum level = spdlog::level::info;

                switch (priority) {
                    case SDL_LOG_PRIORITY_TRACE:
                    case SDL_LOG_PRIORITY_VERBOSE: level = spdlog::level::trace; break;
                    case SDL_LOG_PRIORITY_DEBUG: level = spdlog::level::debug; break;
                    case SDL_LOG_PRIORITY_WARN: level = spdlog::level::warn; break;
                    case SDL_LOG_PRIORITY_ERROR: level = spdlog::level::err; break;
                    case SDL_LOG_PRIORITY_CRITICAL: level = spdlog::level::critical; break;
                    default: break;
                }
                logger().log(level, "SDL ({}): {}", category, message);
            },
            nullptr
        );

        windowManager_.init();
        wake_on_log(true);

        const int sdl = SDL_GetVersion();
        logger().info(
            "ironcast {} starting, SDL {}.{}.{}",
            IRONCAST_VERSION,
            SDL_VERSIONNUM_MAJOR(sdl),
            SDL_VERSIONNUM_MINOR(sdl),
            SDL_VERSIONNUM_MICRO(sdl)
        );
    }

    Ironcast& Ironcast::app(int argc, char** argv) {
        static Ironcast instance(argc, argv);
        return instance;
    }

    Ironcast::~Ironcast() {
        wake_on_log(false);
        SDL_SetLogOutputFunction(SDL_GetDefaultLogOutputFunction(), nullptr);
        spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& l) { l->flush(); });
        current_ = nullptr;
    }

    Ironcast& Ironcast::current() {
        IRONCAST_DEBUG_ASSERT(current_ != nullptr, "Ironcast::current called with no Ironcast instance");
        return *current_;
    }

    bool Ironcast::has_current() {
        return current_ != nullptr;
    }

    void Ironcast::destroy() {
        // SDL shuts down with the windows; no more wake events after that.
        wake_on_log(false);
        windowManager_.destroy();
    }

    int Ironcast::exec() {
        init();

#ifdef __EMSCRIPTEN__
        // The browser drives the loop: one update() per animation frame. simulate_infinite_loop unwinds
        // main()'s stack (without running destructors) and its memory is reused, which is why the app is
        // the static from app() and never a local in main().
        emscripten_set_main_loop_arg(
            [](void* arg) {
                auto& app = *static_cast<Ironcast*>(arg);
                app.update();

                if (app.shouldExit_) {
                    emscripten_cancel_main_loop();
                    app.destroy();
                    logger().info("ironcast exiting with code {} after {} frames", app.exitCode_, app.frame_);
                }
            }, this, 0, true
        );
#endif

        while(!shouldExit_) {
            update();
        }

        destroy();
        logger().info("ironcast exiting with code {} after {} frames", exitCode_, frame_);
        return exitCode_;
    }

    void Ironcast::exit() {
        add_action([this]() {
            shouldExit_ = true;
        });
    }

    uint64_t Ironcast::frame() const {
        return frame_;
    }

    const FrameConfig& Ironcast::frame_config() const {
        return windowManager_.frame_config();
    }

    void Ironcast::set_frame_config(const FrameConfig &config) {
        windowManager_.set_frame_config(config);
    }

    const Theme& Ironcast::theme() const {
        return theme_;
    }

    void Ironcast::set_theme(const Theme &theme) {
        theme_ = theme;
        // Deferred: replacing the style mid-frame would be undone by the pops of styles pushed before it.
        add_action([this]() {
            windowManager_.apply_theme(theme_);
        });
    }

    void Ironcast::set_log_config(const LogConfig &config) {
        configure_logging(config);
        // The buffer may be new.
        wake_on_log(true);
    }

    void Ironcast::wake_on_log(bool enabled) {
        if (auto memory = log_memory()) {
            if (enabled) {
                memory->set_on_message([this] { windowManager_.wake(); });
            } else {
                memory->set_on_message({});
            }
        }
    }

    const Font& Ironcast::add_font(Font font) {
        return *fonts_.emplace_back(std::make_unique<Font>(std::move(font)));
    }

    void Ironcast::remove_font(const Font& font) {
        IRONCAST_DEBUG_ASSERT(theme_.font != &font, "remove_font called with the current theme's font");
        IRONCAST_DEBUG_ASSERT(
            std::ranges::any_of(fonts_, [&font](const auto& owned) { return owned.get() == &font; }),
            "remove_font called with a font not from add_font"
        );
        // Queued after any pending set_theme(), whose re-theme drops the font from every window's atlas.
        add_action([this, &font]() {
            std::erase_if(fonts_, [&font](const auto& owned) { return owned.get() == &font; });
        });
    }

    const Image& Ironcast::add_image(Image image) {
        return *images_.emplace_back(std::make_unique<Image>(std::move(image)));
    }

    void Ironcast::remove_image(const Image& image) {
        IRONCAST_DEBUG_ASSERT(
            std::ranges::any_of(images_, [&image](const auto& owned) { return owned.get() == &image; }),
            "remove_image called with an image not from add_image"
        );
        add_action([this, &image]() {
            IRONCAST_DEBUG_ASSERT(background_ != &image, "remove_image called with the app's background");
            IRONCAST_DEBUG_ASSERT(icon_ != &image, "remove_image called with the app's icon");
            // Checked when it runs, after any set_background() queued before it.
            for (size_t i = 0; i < windowManager_.count(); i++) {
                IRONCAST_DEBUG_ASSERT(
                    windowManager_.get_window_at(i)->background() != &image,
                    "remove_image called with an image a window still uses as its background"
                );
            }
            std::erase_if(images_, [&image](const auto& owned) { return owned.get() == &image; });
        });
    }

    void Ironcast::set_background(const Image* image, float opacity) {
        IRONCAST_DEBUG_ASSERT(opacity >= 0.0f && opacity <= 1.0f, "background opacity out of [0, 1]", opacity);
        background_ = image;
        backgroundOpacity_ = opacity;
    }

    const Image* Ironcast::background() const {
        return background_;
    }

    float Ironcast::background_opacity() const {
        return backgroundOpacity_;
    }

    void Ironcast::set_background_painter(std::function<BackgroundPainter()> factory) {
        backgroundPainterFactory_ = std::move(factory);
    }

    const std::function<BackgroundPainter()>& Ironcast::background_painter_factory() const {
        return backgroundPainterFactory_;
    }

    void Ironcast::set_backend_factory(BackendFactory factory) {
        backendFactory_ = std::move(factory);
    }

    std::unique_ptr<GPUBackend> Ironcast::make_backend(const WindowConfig &config) {
        // Checked in release too: without a backend the window would crash in init().
        IRONCAST_ASSERT(static_cast<bool>(backendFactory_), "no backend factory; call set_backend_factory()");
        return backendFactory_(config);
    }

    void Ironcast::set_icon(const Image* image) {
        icon_ = image;
    }

    const Image* Ironcast::icon() const {
        return icon_;
    }

    void Ironcast::request_redraw(int frames) {
        windowManager_.request_redraw(frames);
    }

    bool Ironcast::actions_pending() const {
        return actionManager_.pending();
    }

    void Ironcast::add_action(action &&fn, uint64_t delay) {
        IRONCAST_DEBUG_ASSERT(fn != nullptr, "add_action called with an empty/null action");
        actionManager_.add_action(std::move(fn), delay);
        windowManager_.wake();
    }

    uint64_t Ironcast::add_window(std::unique_ptr<Window> window) {
        return windowManager_.add_window(std::move(window));
    }

    uint64_t Ironcast::add_window(const WindowConfig &config) {
        return windowManager_.add_window(std::make_unique<Window>(make_backend(config)));
    }

    size_t Ironcast::window_count() const {
        return windowManager_.count();
    }

    Window* Ironcast::get_window_at(size_t index) {
        return windowManager_.get_window_at(index);
    }

    Window* Ironcast::get_window(uint64_t id) {
        return windowManager_.get_window(id);
    }

    void Ironcast::remove_window_at(size_t index) {
        windowManager_.remove_window_at(index);
    }

    void Ironcast::remove_window(uint64_t id) {
        windowManager_.remove_window(id);
    }
}
