#include <utility>
#include <cstring>

#include "ironcast/assert.hpp"

#include "ironcast/window.hpp"
#include "ironcast/ironcast.hpp"
#include "ironcast/inspector.hpp"

namespace ironcast {
    Window::Window(std::unique_ptr<GPUBackend> backend, std::unique_ptr<Widget> widget)
    : id_(Ironcast::random()),
      rootWidget_(widget == nullptr ? std::make_unique<Widget>() : std::move(widget)),
      backend_(std::move(backend)) {}

    Window::~Window() = default;
    Window::Window(Window&& window) noexcept = default;
    Window& Window::operator=(Window&& window) noexcept = default;

    void Window::init() {
        IRONCAST_DEBUG_ASSERT(backend_ != nullptr, "Window::init called with no backend");
        IRONCAST_DEBUG_ASSERT(rootWidget_ != nullptr, "Window::init called with no root widget");
        backend_->set_window(this);
        rootWidget_->set_window(this);

        // The app's default background, unless one was set on this window before it was created.
        if (backend_->background() == nullptr && app().background() != nullptr) {
            set_background(app().background(), app().background_opacity());
        }

        backend_->init();
        // Uploads the background chosen before the backend existed.
        backend_->update_background();

        // Its own icon if set before it was created, else the app's.
        if (const Image* icon = backend_->icon() != nullptr ? backend_->icon() : app().icon()) {
            set_icon(icon);
        }

        // Likewise its background painter, made for this window.
        if (!backend_->has_background_painter() && app().background_painter_factory()) {
            set_background_painter(app().background_painter_factory()());
        }

        // Last, so widgets can create what they need in the backend. Pairs with destroy().
        rootWidget_->init();
    }

    void Window::destroy() {
        IRONCAST_DEBUG_ASSERT(backend_ != nullptr, "Window::destroy called with no backend");
        IRONCAST_DEBUG_ASSERT(rootWidget_ != nullptr, "Window::destroy called with no root widget");
        // Before the backend, so widgets can still release what they hold in it.
        rootWidget_->destroy();
        backend_->destroy();
    }

    void Window::update() {
        IRONCAST_DEBUG_ASSERT(backend_ != nullptr, "Window::update called with no backend");
        IRONCAST_DEBUG_ASSERT(rootWidget_ != nullptr, "Window::update called with no root widget");
        backend_->update(rootWidget_.get());
    }

    void Window::close() {
        IRONCAST_DEBUG_ASSERT(manager_ != nullptr, "Window::close called before set_window_manager");
        manager_->remove_window(id_);
    }

    void Window::post(fu2::unique_function<void(Window&)> fn, uint64_t delay) {
        app().add_action([id = id_, fn = std::move(fn)]() mutable {
            if (Window* window = Ironcast::current().get_window(id)) {
                fn(*window);
            }
        }, delay);
    }

    Ironcast& Window::app() {
        IRONCAST_DEBUG_ASSERT(manager_ != nullptr, "Window::app called before set_window_manager");
        return manager_->app();
    }

    uint64_t Window::id() const {
        return id_;
    }

    void Window::set_window_manager(WindowManager* manager) {
        IRONCAST_DEBUG_ASSERT(manager != nullptr, "set_window_manager called with null manager");
        manager_ = manager;
    }

    void Window::set_root_widget(std::unique_ptr<Widget> widget) {
        IRONCAST_DEBUG_ASSERT(widget != nullptr, "set_root_widget called with null widget");

        widget->set_window(this);
        // Through post(): dropped if the window is closed before the swap runs.
        post([widget = std::move(widget)](Window& window) mutable {
            if (window.rootWidget_ != nullptr) {
                window.rootWidget_->destroy();
            }

            window.rootWidget_ = std::move(widget);
            window.rootWidget_->init();
        });
    }

    Widget* Window::root_widget() {
        return rootWidget_.get();
    }

    GPUBackend* Window::backend() {
        return backend_.get();
    }

    void Window::maximize() {
        IRONCAST_DEBUG_ASSERT(backend_ != nullptr, "Window::maximize called with no backend");
        backend_->maximize();
    }

    void Window::minimize() {
        IRONCAST_DEBUG_ASSERT(backend_ != nullptr, "Window::minimize called with no backend");
        backend_->minimize();
    }

    void Window::set_size(int width, int height) {
        IRONCAST_DEBUG_ASSERT(backend_ != nullptr, "Window::set_size called with no backend");
        backend_->set_size(width, height);
    }

    void Window::set_title(const std::string &title) {
        IRONCAST_DEBUG_ASSERT(backend_ != nullptr, "Window::set_title called with no backend");
        backend_->set_title(title);
    }

    const std::string& Window::title() const {
        IRONCAST_DEBUG_ASSERT(backend_ != nullptr, "Window::title called with no backend");
        return backend_->title();
    }

    int Window::height() const {
        IRONCAST_DEBUG_ASSERT(backend_ != nullptr, "Window::height called with no backend");
        return backend_->height();
    }

    int Window::width() const {
        IRONCAST_DEBUG_ASSERT(backend_ != nullptr, "Window::width called with no backend");
        return backend_->width();
    }

    ImFont* Window::load_font(const std::filesystem::path& path) {
        IRONCAST_DEBUG_ASSERT(backend_ != nullptr, "Window::load_font called with no backend");
        backend_->make_current();
        return ImGui::GetIO().Fonts->AddFontFromFileTTF(path.string().c_str());
    }

    ImFont* Window::load_font(std::span<const std::byte> data) {
        IRONCAST_DEBUG_ASSERT(backend_ != nullptr, "Window::load_font called with no backend");
        backend_->make_current();
        // The atlas owns (and frees) its copy of the data.
        void* copy = IM_ALLOC(data.size());
        std::memcpy(copy, data.data(), data.size());
        return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(copy, static_cast<int>(data.size()));
    }

    void Window::set_font(ImFont* font) {
        IRONCAST_DEBUG_ASSERT(backend_ != nullptr, "Window::set_font called with no backend");
        IRONCAST_DEBUG_ASSERT(rootWidget_ != nullptr, "Window::set_font called with no root widget");
        backend_->make_current();
        ImGui::GetIO().FontDefault = font;
        // Every widget is measured with the font, so the whole tree needs a new layout.
        rootWidget_->layout().mark_dirty_tree();
    }

    void Window::set_background(const Image* image, float opacity) {
        IRONCAST_DEBUG_ASSERT(backend_ != nullptr, "Window::set_background called with no backend");
        IRONCAST_DEBUG_ASSERT(opacity >= 0.0f && opacity <= 1.0f, "background opacity out of [0, 1]", opacity);
        backend_->set_background(image, opacity);
    }

    const Image* Window::background() const {
        IRONCAST_DEBUG_ASSERT(backend_ != nullptr, "Window::background called with no backend");
        return backend_->background();
    }

    void Window::set_icon(const Image* image) {
        IRONCAST_DEBUG_ASSERT(backend_ != nullptr, "Window::set_icon called with no backend");
        backend_->set_icon(image);
    }

    const Image* Window::icon() const {
        IRONCAST_DEBUG_ASSERT(backend_ != nullptr, "Window::icon called with no backend");
        return backend_->icon();
    }

    void Window::set_background_painter(BackgroundPainter painter) {
        IRONCAST_DEBUG_ASSERT(backend_ != nullptr, "Window::set_background_painter called with no backend");
        backend_->set_background_painter(std::move(painter));
    }

    void Window::set_inspector(bool open) {
#ifdef IRONCAST_INSPECTOR
        if (!open) {
            inspector_.reset();
        } else if (inspector_ == nullptr) {
            inspector_ = std::make_unique<Inspector>();
        }
#endif
    }

    bool Window::inspector_open() const {
        return inspector_ != nullptr;
    }

    void Window::update_inspector() {
#ifdef IRONCAST_INSPECTOR
        if (ImGui::IsKeyPressed(ImGuiKey_F12, false)) {
            set_inspector(inspector_ == nullptr);
        }
        if (inspector_ != nullptr && rootWidget_ != nullptr && !inspector_->update(*rootWidget_)) {
            inspector_.reset();
        }
#endif
    }
}
