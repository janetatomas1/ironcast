#pragma once

#include <concepts>
#include <memory>
#include <vector>

#include "ironcast/plugin.hpp"

namespace ironcast {
    // Owns the app's plugins. Internal to Ironcast, which forwards what apps need (add_plugin(),
    // remove_plugin(), plugin()).
    class PluginManager {
        Ironcast& app_;
        std::vector<std::unique_ptr<Plugin>> plugins_;

    public:
        explicit PluginManager(Ironcast& app);
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
