
#pragma once

#include <string>
#include <function2/function2.hpp>

#include "ironcast/widget.hpp"

namespace ironcast {
    class RadioGroup;

    // One option of a radio group: selected while *groupValue == value, and clicking it writes
    // value there. Usually made through RadioGroup, which owns the shared int; a standalone
    // RadioButton needs groupValue to outlive it.
    class RadioButton : public Widget {
        int* groupValue_ = nullptr;
        int value_ = 0;
        fu2::unique_function<void()> callback_;

        // A moved RadioGroup points its options at its new selected_.
        friend class RadioGroup;

    public:
        RadioButton(const std::string& title, int* groupValue, int value, Widget* parent = nullptr);
        RadioButton(const std::string& title, int* groupValue, int value,
                    fu2::unique_function<void()> callback, Widget* parent = nullptr);

        // True on the frame it is clicked, even when it was already selected.
        bool show() final;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) final;

        [[nodiscard]] bool selected() const;
        // Writes value to the group without calling the callback.
        void select();

        // Called on the next dispatch (see post()) when a click selects this option; not when it
        // was already selected.
        RadioButton& set_callback(fu2::unique_function<void()> callback);
        // Empty when none is set.
        [[nodiscard]] const fu2::unique_function<void()>& callback() const;
    };
}
