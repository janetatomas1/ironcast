#pragma once

#include <optional>

#include "ironcast/widget.hpp"

namespace ironcast {
    // A line across this widget's Yoga box. With a title it is ImGui's SeparatorText: the title
    // on a horizontal line. Stretches across its parent's cross axis by default (align_self),
    // so a horizontal one fills a column's width and a vertical one a row's height.
    class Separator : public Widget {
        bool vertical_ = false;
        std::optional<float> textAlign_;
        std::optional<float> leftWidth_;

    public:
        Separator(const std::string& title = "", Widget* parent = nullptr);

        // Vertical separators have no title; it is ignored while set.
        [[nodiscard]] bool vertical() const;
        Separator& set_vertical(bool vertical);

        // Where the title sits along the line: 0 left, 0.5 centre, 1 right.
        // Unset = style.SeparatorTextAlign.x.
        [[nodiscard]] std::optional<float> text_align() const;
        Separator& set_text_align(std::optional<float> align);

        // Length of the line left of the title, in points. Overrides text_align() (the title
        // is then aligned left), and keeps as much space free right of the title.
        [[nodiscard]] std::optional<float> left_width() const;
        Separator& set_left_width(std::optional<float> width);

        bool show() final;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) final;
    };
}
