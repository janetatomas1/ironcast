#pragma once

namespace ironcast {
    // How the main loop waits between frames. Vsync is off; DisplayMatched replaces it.
    // The values index the wait table in windowmanager.cpp; keep them in order.
    enum class FramePolicy {
        // Sleeps the rest of a 1 / targetFps frame, at least minSleepMs.
        TargetFps = 0,
        // Sleeps delayMs after every frame, however long the frame took.
        FixedDelay = 1,
        // Never sleeps.
        Uncapped = 2,
        // Sleeps until an event, a redraw request or idleTimeoutMs; runs like TargetFps while redrawing.
        OnDemand = 3,
        // Like TargetFps at the fastest refresh rate among the windows' displays; targetFps when unknown.
        DisplayMatched = 4,
        // targetFps while any ironcast window has keyboard focus, idleFps otherwise.
        FocusBased = 5,
        // targetFps while there is activity (events, redraw requests, pending actions) and for activeTimeoutMs
        // after it, then idleFps until the next event, which restores targetFps at once.
        Adaptive = 6,
    };

    // App-wide frame pacing, set with Ironcast::set_frame_config(). Each field says which policies read it.
    struct FrameConfig {
        FramePolicy policy = FramePolicy::TargetFps;
        // TargetFps, OnDemand while redrawing, DisplayMatched's fallback, FocusBased and Adaptive's full rate.
        int targetFps = 60;
        // Every policy that sleeps to a rate: always yield a little, even when a frame is over budget.
        int minSleepMs = 1;
        // FixedDelay.
        int delayMs = 20;
        // OnDemand: wake at least this often, -1 = only on events.
        int idleTimeoutMs = 500;
        // FocusBased while unfocused, Adaptive while idle.
        int idleFps = 10;
        // Adaptive: how long the full rate lasts after the last activity.
        int activeTimeoutMs = 1000;
    };
}
