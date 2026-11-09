#pragma once

#include <string>

#include "ironcast/widget.hpp"

namespace ironcast {
    // A bar filled to fraction(), drawn at this widget's Yoga box. Intrinsic size is the default
    // field width by the frame height; it has no label.
    class ProgressBar : public Widget {
        float fraction_ = 0.0f;
        std::string overlay_;
        bool indeterminate_ = false;

    public:
        ProgressBar(Widget* parent = nullptr);

        // 0..1; values outside are clamped when drawn.
        [[nodiscard]] float fraction() const;
        ProgressBar& set_fraction(float fraction);

        // Text drawn over the bar. Empty = the fraction as a percentage (none when indeterminate).
        [[nodiscard]] const std::string& overlay() const;
        ProgressBar& set_overlay(const std::string& overlay);

        // An animated bar for work of unknown length; fraction() is ignored while set.
        [[nodiscard]] bool indeterminate() const;
        ProgressBar& set_indeterminate(bool indeterminate);

        bool show() final;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) final;
    };
}
