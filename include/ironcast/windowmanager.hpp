
#pragma once

#include <vector>
#include <memory>

#include "ironcast/window.hpp"
#include "ironcast/frameconfig.hpp"


namespace ironcast {
    class Ironcast;

    // Owns the app's windows and runs the frame loop's per-window part: waits as FrameConfig says, polls
    // SDL events, routes them to their windows and draws each window. Internal to Ironcast, which forwards
    // what apps need.
    class WindowManager {
        std::vector<std::unique_ptr<Window>> windows_;
        Ironcast &app_;
        FrameConfig frameConfig_;
        uint64_t frameStartNs_ = 0;
        // When the last event was polled, for Adaptive.
        uint64_t lastEventNs_ = 0;
        uint32_t wakeEvent_ = 0;
        // Frames OnDemand still draws without waiting; starts nonzero so new windows get drawn.
        int redrawFrames_ = 2;

        // Waits as frameConfig_.policy says, before the frame's events are polled.
        void wait();
        // The fastest refresh rate among the windows' displays, 0 when unknown.
        [[nodiscard]] float refresh_rate() const;
        // Whether any window has keyboard focus.
        [[nodiscard]] bool focused() const;

    public:
        WindowManager(Ironcast &app);
        void init();
        // One frame: wait, events, then every window's update().
        void update();
        void destroy();
        // Wakes a loop sleeping in wait(). Thread-safe.
        void wake();
        [[nodiscard]] const FrameConfig& frame_config() const;
        void set_frame_config(const FrameConfig &config);
        // Re-themes every open window; call between frames.
        void apply_theme(const Theme &theme);
        // Keeps an OnDemand loop drawing for at least this many more frames.
        void request_redraw(int frames = 2);
        // Whether the next frame should be drawn without waiting: a redraw was requested or actions are pending.
        [[nodiscard]] bool redraw_pending() const;
        [[nodiscard]] size_t count() const;
        Ironcast &app();
        uint64_t add_window(std::unique_ptr<Window> window);
        Window* get_window_at(size_t index);
        Window* get_window(uint64_t id);
        void remove_window_at(size_t index);
        void remove_window(uint64_t id);
    };
}
