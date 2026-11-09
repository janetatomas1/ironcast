#pragma once

#include <string>

#include "ironcast/widget.hpp"

namespace ironcast {
    // Read-only text drawn with ImGui::TextUnformatted; the title is the text, and "##" is shown as
    // is. It wraps at its Yoga width when it has one, otherwise at the window edge, and its
    // measured height follows the width it gets.
    class Text : public Widget {
    public:
        Text(const std::string& text = "", Widget* parent = nullptr);

        // Always false.
        bool show() final;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) final;
    };
}
