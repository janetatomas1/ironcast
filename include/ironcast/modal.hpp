#pragma once

#include "ironcast/popup.hpp"

namespace ironcast {
    // A Popup with a title bar that blocks input to the rest of the window. Clicking outside
    // doesn't close it; only close() does.
    class Modal : public Popup {
    protected:
        bool begin(ImGuiWindowFlags flags) override;

    public:
        Modal(const std::string& title = "", Widget* parent = nullptr);
    };
}
