
#include <vector>
#include <moodycamel/concurrentqueue.h>

#include "ironcast/action.hpp"

namespace ironcast {
    // Runs functions on the UI thread between frames. Actions can be added from any thread; delayed ones
    // wait in a ring of maxDelayFrames slots. Ironcast owns one; use Ironcast::add_action() or Widget::post().
    class ActionManager {
        moodycamel::ConcurrentQueue<std::pair<action, uint64_t>> immediate_;
        std::vector<std::vector<action>> ring_;
        uint32_t frameIndex_ = 0;
        size_t delayed_ = 0;

    public:
        ActionManager(uint64_t maxDelayFrames = 256);
        // delay is the number of dispatches to wait: 0 runs on the next dispatch, 1 on the one after, and so on.
        // The largest delay is maxDelayFrames - 1; a longer one runs at that limit. Called from a running action,
        // delay 0 runs in the same dispatch, so an action that re-posts itself needs delay 1 (once per dispatch)
        // or it never finishes.
        void add_action(action &&fn, uint64_t delay = 0);
        // Runs the delayed actions that are due, then the queued ones in the order they were added, including any
        // queued meanwhile. Called once per frame by Ironcast.
        void dispatch();
        // Whether any action is queued or waiting in the delay ring.
        [[nodiscard]] bool pending() const;
    };
}
