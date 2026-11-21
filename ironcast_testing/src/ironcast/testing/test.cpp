#include "ironcast/testing/test.hpp"

namespace ironcast::testing {
    const std::string& Test::name() const {
        return name_;
    }

    WindowConfig Test::config() const {
        // Fixed size, not maximized, so results don't depend on the machine.
        WindowConfig config;
        config.width = 1280;
        config.height = 720;
        config.title = name_;
        config.windowState = WindowState::NORMAL;
        return config;
    }
}
