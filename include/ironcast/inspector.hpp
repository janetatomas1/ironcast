#pragma once

#include <cstdint>

namespace ironcast {
    class Widget;

    // Devtools-like overlay for one window: the widget tree, the selected widget's computed layout
    // and flex properties, and its Yoga box drawn over the app. Only built with IRONCAST_INSPECTOR;
    // opened through Window::set_inspector() or F12.
    class Inspector {
        // Ids, not pointers, so a widget destroyed meanwhile just drops out of the inspector.
        uint64_t selected_ = 0;
        uint64_t hovered_ = 0;

        void tree(Widget& widget, bool root);
        void properties(Widget& widget);

    public:
        // Draws the inspector window and the box overlay. Call inside the frame, after the root
        // widget's ImGui window has ended. False once the inspector window is closed.
        bool update(Widget& root);
    };
}
