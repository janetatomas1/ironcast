
#pragma once

#include <functional>
#include <string>

#include "ironcast/widget.hpp"

namespace ironcast {
    // A checkbox with the title as its label. Unchecked by default.
    class Checkbox : public Widget {
        bool value_ = false;
        std::function<void(bool)> onChange_;

    public:
        Checkbox(
            std::string label,
            std::function<void(bool)> on_change = nullptr,
            Widget* parent = nullptr
        );

        // True on the frame the user toggles it.
        bool show() final;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) final;
        [[nodiscard]] bool value() const;
        Checkbox& set_value(bool value);
        // Called on the next dispatch (see post()) with the new value after the user changes it;
        // set_value() doesn't call it.
        Checkbox& set_on_change(std::function<void(bool)> callback);
    };
}
