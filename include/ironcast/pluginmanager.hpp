#pragma once

#include <concepts>
#include <memory>
#include <vector>

#include <function2/function2.hpp>

#include "ironcast/action.hpp"
#include "ironcast/plugin.hpp"

namespace ironcast {
    // Owns the app's plugins. Internal to Ironcast, which forwards what apps need (add_plugin(),
    // remove_plugin(), plugin()).
    class PluginManager {
    public:
        // Queues an action for the next dispatch (Ironcast::add_action()).
        using Post = fu2::unique_function<void(action&&)>;
        // Installs a plugin (Ironcast calls Plugin::install() with itself).
        using Install = fu2::unique_function<void(Plugin&)>;

    private:
        Post post_;
        Install install_;
        std::vector<std::unique_ptr<Plugin>> plugins_;

    public:
        // Takes the app's hooks rather than the app, so it can be tested without one.
        PluginManager(Post post, Install install);
        PluginManager(const PluginManager&) = delete;
        PluginManager& operator=(const PluginManager&) = delete;

        // Keeps the plugin and installs it on the next dispatch, so it can be added from a slot. get() finds it
        // once installed.
        Plugin& add(std::unique_ptr<Plugin> plugin);
        // Destroys the plugin on the next dispatch, so a plugin can remove itself from a slot. Does nothing if
        // it has already been removed.
        void remove(Plugin& plugin);
        // The first plugin of type P, nullptr if there is none.
        template<std::derived_from<Plugin> P>
        P* get();
        // Destroys every plugin, newest first. Called when the app exits, before the windows go.
        void destroy();
    };

    template<std::derived_from<Plugin> P>
    P* PluginManager::get() {
        for (auto& plugin: plugins_) {
            if (auto* p = dynamic_cast<P*>(plugin.get())) {
                return p;
            }
        }
        return nullptr;
    }
}
