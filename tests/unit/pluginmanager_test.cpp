// PluginManager: deferred add and remove, lookup by type, and when plugins and their connections go away.

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include <ironcast/actionmanager.hpp>
#include <ironcast/pluginmanager.hpp>
#include <ironcast/signal.hpp>

using namespace ironcast;

namespace {
    // Records its installs and destruction in a shared log, and counts calls from the signal it connects to.
    struct Probe : Plugin {
        std::vector<std::string>& log;
        std::string name;
        Signal<>* signal = nullptr;
        int calls = 0;

        Probe(std::vector<std::string>& log, std::string name, Signal<>* signal = nullptr)
            : log(log), name(std::move(name)), signal(signal) {}

        ~Probe() override {
            log.push_back(name + " destroyed");
        }

        // Called by the fixture's install hook, standing in for install(Ironcast&).
        void installed() {
            log.push_back(name + " installed");
            if (signal != nullptr) {
                add_connection(signal->connect([this] { ++calls; }));
            }
        }
    };

    struct OtherProbe : Probe {
        using Probe::Probe;
    };

    // A PluginManager driven by a bare ActionManager: dispatch() stands in for a frame.
    struct PluginManagerTest : ::testing::Test {
        std::vector<std::string> log;
        ActionManager actions;
        PluginManager plugins = PluginManager(
            [this](action&& fn) { actions.add_action(std::move(fn)); },
            [](Plugin& plugin) { static_cast<Probe&>(plugin).installed(); }
        );

        Probe& add(std::string name, Signal<>* signal = nullptr) {
            return static_cast<Probe&>(plugins.add(std::make_unique<Probe>(log, std::move(name), signal)));
        }
    };
}

TEST_F(PluginManagerTest, AddInstallsOnNextDispatch) {
    Probe& probe = add("a");
    EXPECT_EQ(probe.name, "a");
    EXPECT_TRUE(log.empty());
    EXPECT_EQ(plugins.get<Probe>(), nullptr);

    actions.dispatch();
    EXPECT_EQ(log, std::vector<std::string>{"a installed"});
    EXPECT_EQ(plugins.get<Probe>(), &probe);
}

TEST_F(PluginManagerTest, GetFindsFirstOfType) {
    Probe& first = add("a");
    add("b");
    auto& other = static_cast<OtherProbe&>(plugins.add(std::make_unique<OtherProbe>(log, "c")));
    actions.dispatch();

    EXPECT_EQ(plugins.get<Probe>(), &first);
    EXPECT_EQ(plugins.get<OtherProbe>(), &other);
    EXPECT_EQ(plugins.get<Plugin>(), &first);
}

TEST_F(PluginManagerTest, GetReturnsNullForMissingType) {
    add("a");
    actions.dispatch();
    EXPECT_EQ(plugins.get<OtherProbe>(), nullptr);
}

TEST_F(PluginManagerTest, RemoveDestroysOnNextDispatch) {
    Probe& probe = add("a");
    actions.dispatch();

    plugins.remove(probe);
    EXPECT_EQ(log, std::vector<std::string>{"a installed"});
    EXPECT_EQ(plugins.get<Probe>(), &probe);

    actions.dispatch();
    EXPECT_EQ(log, (std::vector<std::string>{"a installed", "a destroyed"}));
    EXPECT_EQ(plugins.get<Probe>(), nullptr);
}

TEST_F(PluginManagerTest, RemoveTwiceIsNoOp) {
    Probe& probe = add("a");
    actions.dispatch();

    plugins.remove(probe);
    plugins.remove(probe);
    actions.dispatch();
    EXPECT_EQ(log, (std::vector<std::string>{"a installed", "a destroyed"}));
}

TEST_F(PluginManagerTest, RemoveBeforeInstallRunsAfterIt) {
    Probe& probe = add("a");
    plugins.remove(probe);

    actions.dispatch();
    EXPECT_EQ(log, (std::vector<std::string>{"a installed", "a destroyed"}));
    EXPECT_EQ(plugins.get<Probe>(), nullptr);
}

TEST_F(PluginManagerTest, RemoveKeepsOthers) {
    Probe& a = add("a");
    Probe& b = add("b");
    actions.dispatch();

    plugins.remove(a);
    actions.dispatch();
    EXPECT_EQ(plugins.get<Probe>(), &b);
}

TEST_F(PluginManagerTest, DestroyNewestFirst) {
    add("a");
    add("b");
    add("c");
    actions.dispatch();
    log.clear();

    plugins.destroy();
    EXPECT_EQ(log, (std::vector<std::string>{"c destroyed", "b destroyed", "a destroyed"}));
    EXPECT_EQ(plugins.get<Probe>(), nullptr);
}

TEST_F(PluginManagerTest, ConnectionsDroppedOnRemove) {
    Signal<> signal;
    Probe& probe = add("a", &signal);
    actions.dispatch();

    signal();
    EXPECT_EQ(probe.calls, 1);

    plugins.remove(probe);
    actions.dispatch();
    EXPECT_EQ(signal.slot_count(), 0u);
    signal();   // must not reach the destroyed plugin
}

// The plugin outlives the signal it connected to (e.g. a window's on_draw): destroying it later is safe.
TEST_F(PluginManagerTest, SignalDiesFirst) {
    auto signal = std::make_unique<Signal<>>();
    Probe& probe = add("a", signal.get());
    actions.dispatch();

    (*signal)();
    EXPECT_EQ(probe.calls, 1);

    signal.reset();
    plugins.remove(probe);
    actions.dispatch();
    EXPECT_EQ(log.back(), "a destroyed");
}

// Added from a slot: installed (and connected) after the emit, not during it.
TEST_F(PluginManagerTest, AddFromSlot) {
    Signal<> signal;
    Probe* added = nullptr;
    Connection connection = signal.connect([&] {
        if (added == nullptr) {
            added = &add("a", &signal);
        }
    });

    signal();
    ASSERT_NE(added, nullptr);
    EXPECT_TRUE(log.empty());
    EXPECT_EQ(signal.slot_count(), 1u);

    actions.dispatch();
    EXPECT_EQ(log, std::vector<std::string>{"a installed"});
    EXPECT_EQ(signal.slot_count(), 2u);

    signal();
    EXPECT_EQ(added->calls, 1);
}

// A plugin removes itself from its own slot: it lives through the emit and goes on the next dispatch.
TEST_F(PluginManagerTest, RemoveSelfFromSlot) {
    Signal<> signal;
    Probe& probe = add("a", &signal);
    actions.dispatch();

    Connection connection = signal.connect([&] { plugins.remove(probe); });
    signal();
    EXPECT_EQ(probe.calls, 1);
    EXPECT_EQ(log.back(), "a installed");

    actions.dispatch();
    EXPECT_EQ(log.back(), "a destroyed");
    EXPECT_EQ(signal.slot_count(), 1u);
}

// Plugins still pending install when the manager goes away are destroyed with the queued action, not leaked.
TEST(PluginManager, PendingDestroyedWithQueue) {
    std::vector<std::string> log;
    {
        ActionManager actions;
        PluginManager plugins(
            [&](action&& fn) { actions.add_action(std::move(fn)); },
            [](Plugin& plugin) { static_cast<Probe&>(plugin).installed(); }
        );
        plugins.add(std::make_unique<Probe>(log, "a"));
    }
    EXPECT_EQ(log, std::vector<std::string>{"a destroyed"});
}
