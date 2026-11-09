// ActionManager: when queued actions run, relative to dispatch() calls.

#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <vector>

#include <ironcast/actionmanager.hpp>

using namespace ironcast;

TEST(ActionManager, StartsIdle) {
    ActionManager manager;
    EXPECT_FALSE(manager.pending());
    manager.dispatch();
    EXPECT_FALSE(manager.pending());
}

TEST(ActionManager, ImmediateRunsOnNextDispatch) {
    ActionManager manager;
    int runs = 0;
    manager.add_action([&] { ++runs; });
    EXPECT_EQ(runs, 0);
    EXPECT_TRUE(manager.pending());

    manager.dispatch();
    EXPECT_EQ(runs, 1);
    EXPECT_FALSE(manager.pending());

    manager.dispatch();
    EXPECT_EQ(runs, 1);
}

TEST(ActionManager, ImmediateRunInCallOrder) {
    ActionManager manager;
    std::vector<int> order;
    for (int i = 0; i < 5; ++i) {
        manager.add_action([&order, i] { order.push_back(i); });
    }
    manager.dispatch();
    EXPECT_EQ(order, (std::vector<int>{0, 1, 2, 3, 4}));
}

TEST(ActionManager, DelayCountsDispatches) {
    ActionManager manager;
    int dispatches = 0;
    std::vector<int> ranAt(4, -1);
    for (int delay = 0; delay < 4; ++delay) {
        manager.add_action([&, delay] { ranAt[delay] = dispatches; }, delay);
    }
    for (; dispatches < 6; ++dispatches) {
        manager.dispatch();
    }
    EXPECT_EQ(ranAt, (std::vector<int>{0, 1, 2, 3}));
}

TEST(ActionManager, PendingWhileDelayedActionWaits) {
    ActionManager manager;
    bool ran = false;
    manager.add_action([&] { ran = true; }, 2);

    manager.dispatch();
    EXPECT_FALSE(ran);
    EXPECT_TRUE(manager.pending());
    manager.dispatch();
    EXPECT_FALSE(ran);
    EXPECT_TRUE(manager.pending());
    manager.dispatch();
    EXPECT_TRUE(ran);
    EXPECT_FALSE(manager.pending());
}

TEST(ActionManager, DelayedActionsInSameSlotRunInCallOrder) {
    ActionManager manager;
    std::vector<int> order;
    for (int i = 0; i < 3; ++i) {
        manager.add_action([&order, i] { order.push_back(i); }, 1);
    }
    manager.dispatch();
    EXPECT_TRUE(order.empty());
    manager.dispatch();
    EXPECT_EQ(order, (std::vector<int>{0, 1, 2}));
}

TEST(ActionManager, DelayIsRelativeToTheDispatchThatQueuesIt) {
    ActionManager manager(4);
    int dispatches = 0;
    int ranAt = -1;
    // advance the ring so the slot index wraps
    for (; dispatches < 3; ++dispatches) {
        manager.dispatch();
    }
    manager.add_action([&] { ranAt = dispatches; }, 2);
    for (; dispatches < 10; ++dispatches) {
        manager.dispatch();
    }
    EXPECT_EQ(ranAt, 5);
}

TEST(ActionManager, LargestDelayIsRingSizeMinusOne) {
    ActionManager manager(4);
    int dispatches = 0;
    int ranAt = -1;
    manager.add_action([&] { ranAt = dispatches; }, 3);
    for (; dispatches < 8; ++dispatches) {
        manager.dispatch();
    }
    EXPECT_EQ(ranAt, 3);
}

TEST(ActionManager, LongerDelayRunsAtTheLimit) {
    ActionManager manager(4);
    int dispatches = 0;
    int ranAt = -1;
    manager.add_action([&] { ranAt = dispatches; }, 10);
    for (; dispatches < 8; ++dispatches) {
        manager.dispatch();
    }
    EXPECT_EQ(ranAt, 3);
}

TEST(ActionManager, RunsMoveOnlyActions) {
    ActionManager manager;
    auto value = std::make_unique<int>(42);
    int seen = 0;
    manager.add_action([&seen, value = std::move(value)] { seen = *value; });
    manager.dispatch();
    EXPECT_EQ(seen, 42);
}

TEST(ActionManager, ActionQueuedDuringDispatchRunsInTheSameDispatch) {
    ActionManager manager;
    std::vector<int> order;
    manager.add_action([&] {
        order.push_back(0);
        manager.add_action([&] { order.push_back(1); });
    });
    manager.dispatch();
    EXPECT_EQ(order, (std::vector<int>{0, 1}));
}

TEST(ActionManager, RepostWithDelayOneRunsOncePerDispatch) {
    ActionManager manager;
    int runs = 0;
    std::function<void()> tick = [&] {
        ++runs;
        manager.add_action([&] { tick(); }, 1);
    };
    manager.add_action([&] { tick(); });
    for (int i = 1; i <= 5; ++i) {
        manager.dispatch();
        EXPECT_EQ(runs, i);
    }
}
