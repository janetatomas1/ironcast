#include <algorithm>

#include "ironcast/assert.hpp"

#include "ironcast/actionmanager.hpp"

namespace ironcast {
    ActionManager::ActionManager(uint64_t maxDelayFrames) {
        IRONCAST_DEBUG_ASSERT(maxDelayFrames > 0, "ActionManager requires at least 1 ring slot", maxDelayFrames);

        ring_.resize(maxDelayFrames);
        for (auto& bucket : ring_) {
            bucket.reserve(16);
        }
    }

    void ActionManager::add_action(action &&fn, uint64_t delay) {
        IRONCAST_DEBUG_ASSERT(fn != nullptr, "add_action called with an empty/null action");
        immediate_.enqueue({std::move(fn), delay});
    }

    void ActionManager::dispatch() {
        IRONCAST_DEBUG_ASSERT(!ring_.empty(), "dispatch called on ActionManager with empty ring");
        IRONCAST_DEBUG_ASSERT(frameIndex_ < ring_.size(), "frameIndex_ out of range", frameIndex_, ring_.size());

        // Delayed actions that are due run first, so anything they queue (with any delay) is placed below,
        // in this same dispatch: a re-post with delay 1 runs once per dispatch, wherever it comes from.
        auto& bucket = ring_[frameIndex_];
        delayed_ -= bucket.size();
        for (auto& act : bucket) {
            IRONCAST_DEBUG_ASSERT(act != nullptr, "delayed bucket contains a null/empty action");
            act();
        }
        bucket.clear();

        // Actions queued while these run are dequeued by this same loop.
        std::pair<action, uint64_t> queued;
        while (immediate_.try_dequeue(queued)) {
            IRONCAST_DEBUG_ASSERT(queued.first != nullptr, "dequeued a null/empty action");

            if (queued.second == 0) {
                queued.first();
            } else {
                // The bucket at frameIndex_ has already run, so delay d lands d slots ahead of it. Longer delays
                // than the ring run at its limit, as documented.
                const size_t offset = std::min<uint64_t>(queued.second, ring_.size() - 1);
                const size_t index = (frameIndex_ + offset) % ring_.size();
                IRONCAST_DEBUG_ASSERT(index < ring_.size(), "computed ring index out of range", index, ring_.size());
                ring_[index].push_back(std::move(queued.first));
                delayed_ += 1;
            }
        }

        frameIndex_ = (frameIndex_ + 1) % ring_.size();
    }

    bool ActionManager::pending() const {
        return delayed_ > 0 || immediate_.size_approx() > 0;
    }
}
