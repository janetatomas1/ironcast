#pragma once

#include <vector>

#include "ironcast/signal.hpp"

namespace ironcast {
    class Ironcast;

    // Extends the app through its signals. Add it with Ironcast::add_plugin(); the app owns it. Connections
    // passed to add_connection() are dropped when the plugin is destroyed. UI thread only, like the rest of the app.
    class Plugin {
        std::vector<Connection> connections_;

    public:
        Plugin() = default;
        virtual ~Plugin() = default;
        // Connections refer to the plugin, so it must not move.
        Plugin(const Plugin&) = delete;
        Plugin& operator=(const Plugin&) = delete;
        Plugin(Plugin&&) = delete;
        Plugin& operator=(Plugin&&) = delete;

        // Once, when added: connect to the app's signals here.
        virtual void install(Ironcast& app);

    protected:
        // Keeps the connection until the plugin is destroyed.
        void add_connection(Connection connection);
    };
}
