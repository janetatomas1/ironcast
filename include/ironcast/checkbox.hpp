
#pragma once

#include <function2/function2.hpp>
#include <string>

#include "ironcast/widget.hpp"

namespace ironcast {
    // A checkbox with the title as its label. Unchecked by default.
    class Checkbox : public Widget {
        bool value_ = false;
        fu2::unique_function<void(bool)> callback_;

    public:
        Checkbox(
            std::string label,
            fu2::unique_function<void(bool)> callback = nullptr,
            Widget* parent = nullptr
        );

        // True on the frame the user toggles it.
        bool show() final;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) final;
        [[nodiscard]] bool value() const;
        Checkbox& set_value(bool value);
        // Called on the next dispatch (see post()) with the new value after the user changes it;
        // set_value() doesn't call it.
        Checkbox& set_callback(fu2::unique_function<void(bool)> callback);
        // Empty when none is set.
        [[nodiscard]] const fu2::unique_function<void(bool)>& callback() const;
    };
}
