#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>

#include "ironcast/widget.hpp"
#include "ironcast/ironcast.hpp"

namespace ironcast {
    using ColorPickerFlags = ImGuiColorEditFlags;

    namespace detail {
        // Options ImGui stores globally, e.g. from a picker's right-click menu. Current ImGui context.
        ImGuiColorEditFlags color_edit_options();
    }

    // A full color picker: SV square, hue bar (or wheel), optional alpha bar, a side preview and
    // input rows below. RGB (N == 3) or RGBA (N == 4, the default), each component in 0..1.
    // With a reference color the side preview also shows it as "Original"; clicking it restores it.
    // Opaque black by default. Its height follows its width: the SV square takes what the bars leave.
    template <std::size_t N = 4>
    class ColorPicker : public Widget {
        static_assert(N == 3 || N == 4, "ColorPicker supports 3 (RGB) or 4 (RGBA) components");

    public:
        using Value = std::array<float, N>;

    private:
        // Opaque black.
        Value value_ = [] {
            Value value{};
            if constexpr (N == 4) {
                value[3] = 1.0f;
            }
            return value;
        }();
        std::optional<Value> reference_;
        ColorPickerFlags flags_ = ImGuiColorEditFlags_None;
        std::function<void(const Value&)> onChange_;
        // Whether the last measure() laid out an alpha bar; the right-click menu can toggle it.
        bool measuredAlphaBar_ = false;

        // Flags as ImGui sees them: ColorPicker3 is ColorPicker4 with NoAlpha.
        [[nodiscard]] ColorPickerFlags picker_flags() const;
        [[nodiscard]] bool alpha_bar() const;
        // Space right of the picker block: the side preview group, or only the label with NoSidePreview.
        [[nodiscard]] float side_extent() const;
        [[nodiscard]] float side_height() const;
        // One ColorEdit row per display flag below the picker; all three when none is set.
        [[nodiscard]] int input_rows() const;

    public:
        ColorPicker(const std::string& title = "", Widget* parent = nullptr);

        // True on the frames the colour changes.
        bool show() final;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) final;

        [[nodiscard]] const Value& value() const;
        ColorPicker& set_value(const Value& value);

        [[nodiscard]] const std::optional<Value>& reference() const;
        ColorPicker& set_reference(const Value& reference);
        ColorPicker& clear_reference();

        [[nodiscard]] ColorPickerFlags flags() const;
        ColorPicker& set_flags(ColorPickerFlags flags);

        // Called on the next dispatch (see post()) with the new value after the user changes it;
        // set_value() doesn't call it.
        ColorPicker& set_on_change(std::function<void(const Value&)> callback);
    };

    template <std::size_t N>
    ColorPicker<N>::ColorPicker(const std::string& title, Widget* parent)
    : Widget(title, parent) {
        layout().enable_measure();
    }

    template <std::size_t N>
    ColorPickerFlags ColorPicker<N>::picker_flags() const {
        return N == 3 ? (flags_ | ImGuiColorEditFlags_NoAlpha) : flags_;
    }

    template <std::size_t N>
    bool ColorPicker<N>::alpha_bar() const {
        // Mirrors ImGui::ColorPicker4: without NoOptions the stored options can add the alpha bar.
        const ColorPickerFlags flags = picker_flags();
        const bool stored = !(flags & ImGuiColorEditFlags_NoOptions)
            && (detail::color_edit_options() & ImGuiColorEditFlags_AlphaBar);

        return ((flags & ImGuiColorEditFlags_AlphaBar) || stored) && !(flags & ImGuiColorEditFlags_NoAlpha);
    }

    template <std::size_t N>
    float ColorPicker<N>::side_extent() const {
        const ColorPickerFlags flags = picker_flags();
        const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
        const float label = (flags & ImGuiColorEditFlags_NoLabel) ? 0.0f : label_width();

        if (flags & ImGuiColorEditFlags_NoSidePreview) {
            return label > 0.0f ? spacing + label : 0.0f;
        }

        // Label (or "Current" with NoLabel), a 3x2 frame-height preview, and "Original" with a reference.
        float group = std::max(label, ImGui::GetFrameHeight() * 3.0f);
        if (flags & ImGuiColorEditFlags_NoLabel) {
            group = std::max(group, ImGui::CalcTextSize("Current").x);
        }
        if (reference_) {
            group = std::max(group, ImGui::CalcTextSize("Original").x);
        }

        return spacing + group;
    }

    template <std::size_t N>
    float ColorPicker<N>::side_height() const {
        const ColorPickerFlags flags = picker_flags();
        const float label = (flags & ImGuiColorEditFlags_NoLabel)
            ? 0.0f
            : ImGui::CalcTextSize(title_id().c_str(), nullptr, true).y;
        const bool hasLabel = label_width() > 0.0f && !(flags & ImGuiColorEditFlags_NoLabel);

        if (flags & ImGuiColorEditFlags_NoSidePreview) {
            return hasLabel ? label : 0.0f;
        }

        // A vertical group: items stacked with ItemSpacing.y between them.
        const float preview = ImGui::GetFrameHeight() * 2.0f;
        float total = 0.0f;
        int items = 0;

        if (hasLabel) {
            total += label;
            ++items;
        }
        if (flags & ImGuiColorEditFlags_NoLabel) {
            total += ImGui::CalcTextSize("Current").y;
            ++items;
        }
        total += preview;
        ++items;
        if (reference_) {
            total += ImGui::CalcTextSize("Original").y + preview;
            items += 2;
        }

        return total + static_cast<float>(items - 1) * ImGui::GetStyle().ItemSpacing.y;
    }

    template <std::size_t N>
    int ColorPicker<N>::input_rows() const {
        const ColorPickerFlags flags = picker_flags();
        if (flags & ImGuiColorEditFlags_NoInputs) {
            return 0;
        }

        const int shown = std::popcount(static_cast<unsigned>(flags & ImGuiColorEditFlags_DisplayMask_));
        return shown == 0 ? 3 : shown;
    }

    template <std::size_t N>
    bool ColorPicker<N>::show() {
        // The right-click menu changes the stored options during show(); lay out again next frame.
        if (alpha_bar() != measuredAlphaBar_) {
            layout().mark_dirty();
        }

        // The item width is the picker block; the side preview and label go right of it.
        set_next_field_width(side_extent());

        float color[4] = {value_[0], value_[1], value_[2], 1.0f};
        float reference[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        if constexpr (N == 4) {
            color[3] = value_[3];
        }
        if (reference_) {
            std::copy(reference_->begin(), reference_->end(), reference);
        }

        const bool changed = ImGui::ColorPicker4(
            title_id().c_str(),
            color,
            picker_flags(),
            reference_ ? reference : nullptr
        );

        if (changed) {
            std::copy(color, color + N, value_.begin());

            if (onChange_) {
                post([value = value_](auto& self) {
                    if (self.onChange_) {
                        self.onChange_(value);
                    }
                });
            }
        }

        return changed;
    }

    template <std::size_t N>
    ImVec2 ColorPicker<N>::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        // Mirrors ImGui::ColorPicker4. The item width is the picker block: SV square, then hue bar and
        // optional alpha bar, each ItemInnerSpacing.x apart. The SV square takes what the bars leave,
        // so height follows width. Input rows go below the taller of the block and the side group.
        const ImGuiStyle& style = ImGui::GetStyle();
        const float bar = ImGui::GetFrameHeight();
        const float bars = alpha_bar() ? 2.0f : 1.0f;
        const float side = side_extent();
        measuredAlphaBar_ = alpha_bar();

        const float total = fit_measure(default_field_width() + side, width, widthMode);
        const float square = std::max(bar, total - side - bars * (bar + style.ItemInnerSpacing.x));
        const float rows = static_cast<float>(input_rows()) * (style.ItemSpacing.y + bar);
        const float intrinsicHeight = std::max(square, side_height()) + rows;

        return ImVec2{total, fit_measure(intrinsicHeight, height, heightMode)};
    }

    template <std::size_t N>
    const typename ColorPicker<N>::Value& ColorPicker<N>::value() const {
        return value_;
    }

    template <std::size_t N>
    ColorPicker<N>& ColorPicker<N>::set_value(const Value& value) {
        value_ = value;
        return *this;
    }

    template <std::size_t N>
    const std::optional<typename ColorPicker<N>::Value>& ColorPicker<N>::reference() const {
        return reference_;
    }

    template <std::size_t N>
    ColorPicker<N>& ColorPicker<N>::set_reference(const Value& reference) {
        const bool added = !reference_;
        reference_ = reference;
        // The "Original" preview adds to the side group.
        if (added) {
            layout().mark_dirty();
        }
        return *this;
    }

    template <std::size_t N>
    ColorPicker<N>& ColorPicker<N>::clear_reference() {
        if (reference_) {
            reference_.reset();
            layout().mark_dirty();
        }
        return *this;
    }

    template <std::size_t N>
    ColorPickerFlags ColorPicker<N>::flags() const {
        return flags_;
    }

    template <std::size_t N>
    ColorPicker<N>& ColorPicker<N>::set_flags(ColorPickerFlags flags) {
        flags_ = flags;
        // NoInputs, NoSidePreview, NoLabel, AlphaBar and the display flags change the measured size.
        layout().mark_dirty();
        return *this;
    }

    template <std::size_t N>
    ColorPicker<N>& ColorPicker<N>::set_on_change(std::function<void(const Value&)> callback) {
        onChange_ = std::move(callback);
        return *this;
    }
}
