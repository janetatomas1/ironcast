#include "ironcast/progressbar.hpp"

namespace ironcast {
    ProgressBar::ProgressBar(Widget* parent): Widget("", parent) {
        layout().enable_measure();
    }

    float ProgressBar::fraction() const {
        return fraction_;
    }

    ProgressBar& ProgressBar::set_fraction(float fraction) {
        fraction_ = fraction;
        return *this;
    }

    const std::string& ProgressBar::overlay() const {
        return overlay_;
    }

    ProgressBar& ProgressBar::set_overlay(const std::string& overlay) {
        overlay_ = overlay;
        return *this;
    }

    bool ProgressBar::indeterminate() const {
        return indeterminate_;
    }

    ProgressBar& ProgressBar::set_indeterminate(bool indeterminate) {
        indeterminate_ = indeterminate;
        return *this;
    }

    bool ProgressBar::show() {
        // ImGui animates a negative fraction; it has to change with time to move.
        const float fraction = indeterminate_ ? -static_cast<float>(ImGui::GetTime()) : fraction_;
        const char* overlay = overlay_.empty() ? nullptr : overlay_.c_str();

        // Takes the size directly, so the Yoga box is drawn as is.
        ImGui::ProgressBar(fraction, size(), overlay);
        return false;
    }

    ImVec2 ProgressBar::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        // Same as ImGui's auto size: item width by frame height. The overlay isn't counted,
        // ImGui clips it to the bar.
        return measure_field(0.0f, width, widthMode, height, heightMode);
    }
}
