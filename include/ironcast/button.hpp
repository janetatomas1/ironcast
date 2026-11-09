
#pragma once

#include <function2/function2.hpp>

#include "ironcast/widget.hpp"

namespace ironcast {
    // A push button drawn with ImGui::Button at its Yoga box. Its natural size is the visible
    // label (text after "##" hidden) plus frame padding.
    class Button: public Widget {
        fu2::unique_function<void()> callback_;

    public:
        Button(
            const std::string &title,
            Widget *parent = nullptr
        );
        Button(
            const std::string &title,
            fu2::unique_function<void()> callback,
            Widget *parent = nullptr
        );
        // True on the frame the button is clicked.
        bool show() final;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) final;
        // Called on the next dispatch after a click, not inside the frame (see post()), and dropped
        // if the button is destroyed first. Empty = none.
        void set_callback(fu2::unique_function<void()> callback);
    };
}
