
#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>

#include <SDL3/SDL.h>
#include <imgui.h>

#include "ironcast/widget.hpp"
#include "ironcast/gpubackend.hpp"

namespace ironcast {
    class Ironcast;
    class WindowManager;
    class Inspector;

    // One OS window with its own GPU and ImGui contexts and a root widget laid out over the whole window.
    // Created through Ironcast::add_window(), which owns it; init(), update() and destroy() are driven by
    // the app.
    class Window {
    protected:
        uint64_t id_ = 0;
        WindowManager *manager_ = nullptr;
        std::unique_ptr<Widget> rootWidget_;
        std::unique_ptr<GPUBackend> backend_;
        // Non-null while the inspector is open.
        std::unique_ptr<Inspector> inspector_;
    public:
        virtual ~Window();
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window &&window) noexcept;
        Window& operator=(Window &&window) noexcept;
        // The backend comes from Ironcast::make_backend() unless the app builds its own.
        Window(std::unique_ptr<GPUBackend> backend, std::unique_ptr<Widget> widget = nullptr);
        void init();
        void destroy();
        void update();
        // Removes the window from the app; it is destroyed on the next frame.
        void close();
        // Runs fn on the next dispatch with this window if it is still open then; dropped if it was closed
        // meanwhile, or if it has not been created yet. Use the window passed to fn, never a captured this.
        // Needs the window added to the app. delay works as in Ironcast::add_action().
        void post(fu2::unique_function<void(Window&)> fn, uint64_t delay = 0);
        Ironcast &app();
        [[nodiscard]] uint64_t id() const;
        void set_window_manager(WindowManager *manager);
        // Takes ownership and gives the widget this window.
        void set_root_widget(std::unique_ptr<Widget> widget);
        Widget* root_widget();
        // The backend drawing this window.
        GPUBackend* backend();
        void maximize();
        void minimize();
        void set_size(int width, int height);
        void set_title(const std::string &title);
        [[nodiscard]] const std::string& title() const;
        [[nodiscard]] int height() const;
        [[nodiscard]] int width() const;
        // Adds a font to this window's atlas, for Style::set_font. nullptr if it can't be loaded.
        // Makes this window's contexts current. Each call adds a new font, so load once and reuse.
        [[nodiscard]] ImFont* load_font(const std::filesystem::path& path);
        // Copies data.
        [[nodiscard]] ImFont* load_font(std::span<const std::byte> data);
        // Primary font for the whole window (ImGuiIO::FontDefault); widget styles still override it.
        // Must come from this window's load_font(). nullptr = ImGui's default font.
        void set_font(ImFont* font);
        // Drawn stretched over the whole window, behind every widget. Use an image from Ironcast::add_image();
        // nullptr removes it. Below opacity 1 the theme's WindowBg shows through.
        void set_background(const Image* image, float opacity = 1.0f);
        [[nodiscard]] const Image* background() const;
        // Custom background drawn every frame on top of the image, Empty removes it.
        void set_background_painter(BackgroundPainter painter);
        // Title bar / taskbar icon, replacing the app's default (Ironcast::set_icon()). Use an image from
        // Ironcast::add_image(); SDL copies it, so the image may be removed afterwards
        // (icon() then dangles). nullptr can't remove an
        // icon once shown, it only stops a later default from replacing it.
        void set_icon(const Image* image);
        [[nodiscard]] const Image* icon() const;
        // Widget inspector overlay, also toggled with F12. Does nothing when ironcast is built with
        // IRONCAST_INSPECTOR=OFF.
        void set_inspector(bool open);
        [[nodiscard]] bool inspector_open() const;
        // Called by the backend each frame, after the root widget's ImGui window has ended.
        void update_inspector();
    };
}
