#pragma once

#include <sigslot/signal.hpp>

namespace ironcast {
    // A signal: connect() slots, call it to emit. UI-thread only, so the single-threaded variant (no mutex per
    // emit). Workers post to the UI thread and emit from there.
    template<typename... Args>
    using Signal = sigslot::signal_st<Args...>;

    // Disconnects when destroyed: Connection c = signal.connect(...). Plugins and widgets keep these.
    using Connection = sigslot::scoped_connection;
}
