#include "ironcast/plugin.hpp"

namespace ironcast {
    void Plugin::install(Ironcast&) {}

    void Plugin::add_connection(Connection connection) {
        connections_.push_back(std::move(connection));
    }
}
