#include <algorithm>
#include <array>

#include <imgui_impl_sdl3.h>
#include <SDL3/SDL.h>
#include "ironcast/assert.hpp"

#include "ironcast/windowmanager.hpp"
#include "ironcast/ironcast.hpp"


namespace ironcast {
    namespace {
        // frameStartNs is when the previous frame started, lastEventNs when the last event was polled.
        // refreshRate and focused are only filled in for the policies that use them; refreshRate is 0 when unknown.
        struct WaitState {
            const FrameConfig &config;
            uint64_t frameStartNs;
            uint64_t lastEventNs;
            bool redrawPending;
            float refreshRate;
            bool focused;
        };
        using WaitFn = void (*)(const WaitState &state);

        void sleep_budget(const WaitState &state, uint64_t budget) {
            const uint64_t minSleep = SDL_MS_TO_NS(static_cast<uint64_t>(state.config.minSleepMs));
            const uint64_t elapsed = SDL_GetTicksNS() - state.frameStartNs;
            const uint64_t sleep = std::max(budget > elapsed ? budget - elapsed : 0, minSleep);

            if (sleep > 0) {
                SDL_DelayPrecise(sleep);
            }
        }

        uint64_t fps_budget(int fps) {
            return SDL_NS_PER_SECOND / static_cast<uint64_t>(fps);
        }

        void wait_target_fps(const WaitState &state) {
            sleep_budget(state, fps_budget(state.config.targetFps));
        }

        void wait_fixed_delay(const WaitState &state) {
            SDL_DelayPrecise(SDL_MS_TO_NS(static_cast<uint64_t>(state.config.delayMs)));
        }

        void wait_uncapped(const WaitState &) {}

        void wait_on_demand(const WaitState &state) {
            if (state.redrawPending) {
                wait_target_fps(state);
            } else {
                // Leaves the event in the queue for the poll in update().
                SDL_WaitEventTimeout(nullptr, state.config.idleTimeoutMs);
            }
        }

        void wait_display_matched(const WaitState &state) {
            if (state.refreshRate > 0.0f) {
                sleep_budget(state, static_cast<uint64_t>(static_cast<double>(SDL_NS_PER_SECOND) / state.refreshRate));
            } else {
                wait_target_fps(state);
            }
        }

        void wait_focus_based(const WaitState &state) {
            // A plain sleep, so a focus change waits for the end of the idle frame.
            sleep_budget(state, fps_budget(state.focused ? state.config.targetFps : state.config.idleFps));
        }

        void wait_adaptive(const WaitState &state) {
            const uint64_t now = SDL_GetTicksNS();
            const uint64_t activeTimeout = SDL_MS_TO_NS(static_cast<uint64_t>(state.config.activeTimeoutMs));

            if (state.redrawPending || now - state.lastEventNs < activeTimeout) {
                wait_target_fps(state);
                return;
            }

            // Idle: wait out the rest of the idle frame, but wake on the first event.
            const uint64_t budget = fps_budget(state.config.idleFps);
            const uint64_t elapsed = now - state.frameStartNs;
            const uint64_t remaining = budget > elapsed ? budget - elapsed : 0;
            SDL_WaitEventTimeout(nullptr, static_cast<Sint32>(SDL_NS_TO_MS(remaining)));
        }

        // Indexed by FramePolicy.
        constexpr std::array<WaitFn, 7> waits = {
            wait_target_fps,
            wait_fixed_delay,
            wait_uncapped,
            wait_on_demand,
            wait_display_matched,
            wait_focus_based,
            wait_adaptive,
        };
        static_assert(static_cast<size_t>(FramePolicy::Adaptive) == waits.size() - 1);
    }

    WindowManager::WindowManager(Ironcast &app) : app_(app) {}

    void WindowManager::init() {
        [[maybe_unused]] bool ok = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS);
        IRONCAST_DEBUG_ASSERT(ok, "SDL_Init failed", SDL_GetError());

        wakeEvent_ = SDL_RegisterEvents(1);
        IRONCAST_DEBUG_ASSERT(wakeEvent_ != 0, "SDL_RegisterEvents failed");
    }

    void WindowManager::wake() {
        SDL_Event event{};
        event.type = wakeEvent_;
        SDL_PushEvent(&event);
    }

    const FrameConfig& WindowManager::frame_config() const {
        return frameConfig_;
    }

    void WindowManager::set_frame_config(const FrameConfig &config) {
        IRONCAST_DEBUG_ASSERT(config.targetFps > 0, "targetFps must be positive", config.targetFps);
        IRONCAST_DEBUG_ASSERT(config.minSleepMs >= 0, "minSleepMs is negative", config.minSleepMs);
        IRONCAST_DEBUG_ASSERT(config.delayMs >= 0, "delayMs is negative", config.delayMs);
        IRONCAST_DEBUG_ASSERT(config.idleTimeoutMs >= -1, "idleTimeoutMs is below -1", config.idleTimeoutMs);
        IRONCAST_DEBUG_ASSERT(config.idleFps > 0, "idleFps must be positive", config.idleFps);
        IRONCAST_DEBUG_ASSERT(config.activeTimeoutMs >= 0, "activeTimeoutMs is negative", config.activeTimeoutMs);
        frameConfig_ = config;
    }

    void WindowManager::request_redraw(int frames) {
        IRONCAST_DEBUG_ASSERT(frames >= 0, "request_redraw called with a negative frame count", frames);
        redrawFrames_ = std::max(redrawFrames_, frames);
    }

    bool WindowManager::redraw_pending() const {
        return redrawFrames_ > 0 || app_.actions_pending();
    }

    void WindowManager::wait() {
        const auto index = static_cast<size_t>(frameConfig_.policy);
        IRONCAST_DEBUG_ASSERT(index < waits.size(), "unknown frame policy", index);

        const float refreshRate = frameConfig_.policy == FramePolicy::DisplayMatched ? refresh_rate() : 0.0f;
        const bool isFocused = frameConfig_.policy == FramePolicy::FocusBased && focused();
        waits[index]({frameConfig_, frameStartNs_, lastEventNs_, redraw_pending(), refreshRate, isFocused});
        frameStartNs_ = SDL_GetTicksNS();
    }

    float WindowManager::refresh_rate() const {
        float rate = 0.0f;

        for (const auto &window: windows_) {
            const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window->backend()->handle()));

            if (mode != nullptr) {
                rate = std::max(rate, mode->refresh_rate);
            }
        }
        return rate;
    }

    bool WindowManager::focused() const {
        return std::any_of(windows_.begin(), windows_.end(), [](const auto &window) {
            return (SDL_GetWindowFlags(window->backend()->handle()) & SDL_WINDOW_INPUT_FOCUS) != 0;
        });
    }

    void WindowManager::destroy() {
        // Windows that are still open when the app exits (e.g. via Ironcast::exit()) have to release their
        // ImGui and GL resources while SDL is still alive.
        for (auto &window: windows_) {
            window->destroy();
        }
        windows_.clear();

        SDL_Quit();
    }

    void WindowManager::update() {
#ifndef __EMSCRIPTEN__
        // In the browser, requestAnimationFrame paces the frames and blocking would freeze the page.
        wait();
#endif
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            // ImGui needs a frame or two after an event to settle hover and layout.
            request_redraw();
            lastEventNs_ = SDL_GetTicksNS();

            // Every window has its own ImGui context, so an event has to reach the context of its window.
            // Events that belong to no window (e.g. gamepad hot-plug) go to all of them.
            SDL_Window* handle = SDL_GetWindowFromEvent(&event);
            Window* target = nullptr;

            if (handle == nullptr) {
                for (auto &window: windows_) {
                    window->backend()->process_event(event);
                }
            } else {
                target = static_cast<Window*>(SDL_GetPointerProperty(SDL_GetWindowProperties(handle), "WRAPPER", nullptr));

                if (target != nullptr) {
                    target->backend()->process_event(event);
                }
            }

            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                IRONCAST_DEBUG_ASSERT(target != nullptr, "close-requested event for untracked window");
                target->close();
            }

            // Debug: a drag-resize sends one per step.
            if (event.type == SDL_EVENT_WINDOW_RESIZED && target != nullptr) {
                logger().debug(
                    "window {} '{}' resized to {}x{}",
                    target->id(),
                    target->title(),
                    event.window.data1,
                    event.window.data2
                );
            }
        }
        for (auto &window: windows_) {
            window->update();
        }
        redrawFrames_ = std::max(redrawFrames_ - 1, 0);
    }

    void WindowManager::apply_theme(const Theme &theme) {
        for (auto &window: windows_) {
            window->backend()->make_current();
            window->backend()->apply_theme(theme);
            // Style vars change measured sizes.
            window->root_widget()->layout().mark_dirty_tree();
        }
        request_redraw();
    }

    size_t WindowManager::count() const {
        return windows_.size();
    }

    Ironcast& WindowManager::app() {
        return app_;
    }

    uint64_t WindowManager::add_window(std::unique_ptr<Window> window) {
        IRONCAST_DEBUG_ASSERT(window != nullptr, "add_window called with a null window");

        const uint64_t id = window->id();
        window->set_window_manager(this);

        // The window is initialized and tracked on the next frame; until then the queued action owns it.
        app_.add_action([window = std::move(window), this] () mutable {
            window->init();
            windows_.push_back(std::move(window));
        });

        return id;
    }

    Window* WindowManager::get_window_at(const size_t index) {
        if (index >= windows_.size()) {
            return nullptr;
        }
        return windows_[index].get();
    }

    Window* WindowManager::get_window(uint64_t id) {
        auto it = std::find_if(windows_.begin(), windows_.end(),
        [id](const auto& window){
            return id == window->id();
        });

        if (it != windows_.end()) {
            return it->get();
        }

        return nullptr;
    }

    void WindowManager::remove_window_at(size_t index) {
        IRONCAST_DEBUG_ASSERT(index < windows_.size(), "window index out of range", index, windows_.size());
        remove_window(windows_[index]->id());
    }

    void WindowManager::remove_window(uint64_t id) {
        app().add_action([this, id](){
            auto it = std::find_if(
                windows_.begin(),
                windows_.end(),
                [id](const auto& win) {
                    return win->id() == id;
                }
            );

            // The window may already be gone (closed by the user, or removed twice); that is not an error.
            if (it == windows_.end()) {
                return;
            }

            (*it)->destroy();
            windows_.erase(it);
        });
    }
}