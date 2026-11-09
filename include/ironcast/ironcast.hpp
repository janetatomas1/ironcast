
#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "ironcast/windowmanager.hpp"
#include "ironcast/randomgenerator.hpp"
#include "ironcast/actionmanager.hpp"
#include "ironcast/theme.hpp"
#include "ironcast/image.hpp"
#include "ironcast/log.hpp"

namespace ironcast {
    // The application: owns the windows, the action queue, the theme, fonts and images, and runs the main
    // loop. Create exactly one, add windows, then return exec() from main(). Most calls are UI thread
    // only; add_action() is the way in from other threads.
    class Ironcast {
        // The live app, so widgets reach it without a window (e.g. from their constructors).
        static inline Ironcast* current_ = nullptr;

        int exitCode_ = 0;
        uint64_t frame_ = 0;
        bool shouldExit_ = false;

        // Before windowManager_, so the fonts outlive the windows whose atlases borrow them. Heap-allocated so
        // the addresses add_font() hands out survive the vector growing.
        std::vector<std::unique_ptr<Font>> fonts_;
        // Like fonts_: before windowManager_, heap-allocated for stable addresses.
        std::vector<std::unique_ptr<Image>> images_;
        const Image* background_ = nullptr;
        float backgroundOpacity_ = 1.0f;
        std::function<BackgroundPainter()> backgroundPainterFactory_;
        BackendFactory backendFactory_;
        const Image* icon_ = nullptr;
        WindowManager windowManager_ = WindowManager(*this);
        static RandomGenerator randomGenerator_;
        ActionManager actionManager_;
        Theme theme_;

        void init();
        void destroy();
        void update();
        // Makes messages logged from any thread wake the loop, so a LogWidget shows them; false undoes it.
        void wake_on_log(bool enabled);
    public:
        Ironcast(int argc, char** argv);
        ~Ironcast();

        // WindowManager and current_ hold this address, so it must not move.
        Ironcast(const Ironcast&) = delete;
        Ironcast& operator=(const Ironcast&) = delete;
        Ironcast(Ironcast&&) = delete;
        Ironcast& operator=(Ironcast&&) = delete;

        // The live app. Asserts when there is none (e.g. headless tests).
        static Ironcast& current();
        [[nodiscard]] static bool has_current();
        // Runs the main loop until exit() or the last window closes; returns the exit code.
        int exec();
        // Ends the loop after the current frame's actions have run.
        void exit();

        // A random 64-bit number, e.g. for ids. UI thread only.
        static uint64_t random() {
            return randomGenerator_.random();
        }
        // Frames run so far.
        [[nodiscard]] uint64_t frame() const;
        [[nodiscard]] const FrameConfig& frame_config() const;
        void set_frame_config(const FrameConfig &config);
        [[nodiscard]] const Theme& theme() const;
        // Applies to every window, open or created later. Open windows switch on the next frame.
        void set_theme(const Theme &theme);
        // See configure_logging(); the app starts with LogConfig's defaults.
        void set_log_config(const LogConfig &config);
        // The app keeps the font until it exits; the reference stays valid, e.g. for Theme::font.
        const Font& add_font(Font font);
        // Frees a font from add_font() on the next frame. It must not be the current theme's font (set another
        // theme first); references to it dangle afterwards.
        void remove_font(const Font& font);
        // The app keeps the image until it exits or remove_image(); windows draw it with set_background().
        const Image& add_image(Image image);
        // Frees an image from add_image() on the next frame. Neither the app nor an open window may use it as
        // its background, nor the app as its icon.
        void remove_image(const Image& image);
        // Default background for windows created afterwards: Window::init() takes it unless the window already
        // has one. Use an image from add_image(); nullptr = none.
        void set_background(const Image* image, float opacity = 1.0f);
        [[nodiscard]] const Image* background() const;
        [[nodiscard]] float background_opacity() const;
        // Makes the background painter for each window created afterwards (painters keep per-window state, so
        // each window gets its own). Window::init() calls it unless the window already has a painter.
        // Empty = none.
        void set_background_painter(std::function<BackgroundPainter()> factory);
        [[nodiscard]] const std::function<BackgroundPainter()>& background_painter_factory() const;
        // Makes the backend of each window created afterwards (opengl::Backend, sdlgpu::Backend, ...). There is
        // no default: the app sets one before adding windows.
        void set_backend_factory(BackendFactory factory);
        // A backend for a new window, from the factory. Asserts when there is no factory.
        [[nodiscard]] std::unique_ptr<GPUBackend> make_backend(const WindowConfig &config);
        // Default icon for windows created afterwards: Window::init() takes it unless the window already has
        // one (Window::set_icon()). Use an image from add_image(); nullptr = none.
        void set_icon(const Image* image);
        [[nodiscard]] const Image* icon() const;
        // Keeps an OnDemand loop drawing for at least this many more frames. Animating widgets call it every
        // frame they are drawn.
        void request_redraw(int frames = 2);
        // Whether any action is queued or delayed.
        [[nodiscard]] bool actions_pending() const;
        // Runs fn on a later dispatch; see ActionManager::add_action() for delay. Thread-safe; also wakes an
        // OnDemand loop.
        void add_action(
            action &&fn,
            uint64_t delay = 0
        );
        // The app takes ownership of the window and returns its id. The window is created on the next frame, so
        // get_window(id) is null until then and again once the window has been closed. Use it to check whether
        // a window is still open.
        uint64_t add_window(std::unique_ptr<Window> window);
        uint64_t add_window(const WindowConfig &config = {});
        // Constructs W (args forwarded) as the root widget of a new window; returns the window's id.
        template<WidgetConcept W, typename... Args>
        uint64_t add_window(WindowConfig config, Args &&... args);
        template<WidgetConcept W, typename... Args>
        uint64_t add_window(Args &&... args);
        // Open windows, for get_window_at().
        [[nodiscard]] size_t window_count() const;
        // nullptr when there is no such window.
        Window* get_window_at(size_t index);
        Window* get_window(uint64_t id);
        void remove_window_at(size_t index);
        // Does nothing if the window has already been closed.
        void remove_window(uint64_t id);
    };

    template <WidgetConcept W, typename ... Args>
    uint64_t Ironcast::add_window(WindowConfig config, Args&&... args) {
        // The window is registered first so it can reach the app; set_root_widget() then gives the
        // widget its window right away and queues the swap after the window's init, which calls
        // the widget's init(). The widget's constructor still runs without a window.
        auto window = std::make_unique<Window>(make_backend(config));
        Window* raw = window.get();
        const uint64_t id = add_window(std::move(window));
        raw->set_root_widget(std::make_unique<W>(std::forward<Args>(args)...));
        return id;
    }

    template <WidgetConcept W, typename ... Args>
    uint64_t Ironcast::add_window(Args&&... args) {
        return add_window<W>({}, std::forward<Args>(args)...);
    }
}
