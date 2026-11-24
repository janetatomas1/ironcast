#include <algorithm>

#include "ironcast/assert.hpp"

#include "ironcast/pluginmanager.hpp"
#include "ironcast/ironcast.hpp"

namespace ironcast {
    PluginManager::PluginManager(Ironcast& app) : app_(app) {}

    Plugin& PluginManager::add(std::unique_ptr<Plugin> plugin) {
        IRONCAST_DEBUG_ASSERT(plugin != nullptr, "PluginManager::add called with a null plugin");

        Plugin& added = *plugin;
        // Installed on the next dispatch, outside any emit: install() connects to signals, and a signal must
        // not get new slots while it is emitting. Until then the queued action owns the plugin.
        app_.add_action([this, plugin = std::move(plugin)]() mutable {
            plugin->install(app_);
            plugins_.push_back(std::move(plugin));
        });
        return added;
    }

    void PluginManager::remove(Plugin& plugin) {
        app_.add_action([this, target = &plugin]() {
            auto it = std::find_if(plugins_.begin(), plugins_.end(), [target](const auto& p) {
                return p.get() == target;
            });

            if (it != plugins_.end()) {
                plugins_.erase(it);
            }
        });
    }

    void PluginManager::destroy() {
        while (!plugins_.empty()) {
            plugins_.pop_back();
        }
    }
}
