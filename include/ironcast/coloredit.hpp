#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <string>

#include "ironcast/widget.hpp"
#include "ironcast/ironcast.hpp"

namespace ironcast {
    using ColorEditFlags = ImGuiColorEditFlags;

    // An RGB (N == 3) or RGBA (N == 4, the default) color, each component in 0..1. Clicking the swatch opens a picker.
    // Input fields and swatch share the field width, the title label follows. Opaque black by default.
    template <std::size_t N = 4>
    class ColorEdit : public Widget {
        static_assert(N == 3 || N == 4, "ColorEdit supports 3 (RGB) or 4 (RGBA) components");

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
        ColorEditFlags flags_ = ImGuiColorEditFlags_None;
        std::function<void(const Value&)> onChange_;

        // label_extent(), or 0 with NoLabel.
        [[nodiscard]] float shown_label_extent() const;

    public:
        ColorEdit(const std::string& title = "", Widget* parent = nullptr);

        // True on the frames the colour changes.
        bool show() final;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) final;

        [[nodiscard]] const Value& value() const;
        ColorEdit& set_value(const Value& value);

        [[nodiscard]] ColorEditFlags flags() const;
        ColorEdit& set_flags(ColorEditFlags flags);

        // Called on the next dispatch (see post()) with the new value after the user changes it;
        // set_value() doesn't call it.
        ColorEdit& set_on_change(std::function<void(const Value&)> callback);
    };

    template <std::size_t N>
    ColorEdit<N>::ColorEdit(const std::string& title, Widget* parent)
    : Widget(title, parent) {
        layout().enable_measure();
    }

    template <std::size_t N>
    float ColorEdit<N>::shown_label_extent() const {
        return (flags_ & ImGuiColorEditFlags_NoLabel) ? 0.0f : label_extent();
    }

    template <std::size_t N>
    bool ColorEdit<N>::show() {
        // The item width covers inputs + swatch; with NoInputs ImGui ignores it.
        if (!(flags_ & ImGuiColorEditFlags_NoInputs)) {
            set_next_field_width(shown_label_extent());
        }

        const std::string& id = title_id();
        bool changed;

        // Also scales the swatch, a frame-height square. It stays pushed while ImGui draws the
        // picker popup, which then uses the same padding.
        const bool pushed = push_frame_height();
        if constexpr (N == 3) {
            changed = ImGui::ColorEdit3(id.c_str(), value_.data(), flags_);
        } else {
            changed = ImGui::ColorEdit4(id.c_str(), value_.data(), flags_);
        }
        pop_frame_height(pushed);

        if (changed && onChange_) {
            post([value = value_](auto& self) {
                if (self.onChange_) {
                    self.onChange_(value);
                }
            });
        }

        return changed;
    }

    template <std::size_t N>
    ImVec2 ColorEdit<N>::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        // Mirrors ImGui::ColorEdit4: inputs + swatch fill the item width, the label follows after ItemInnerSpacing.
        // With NoInputs only the swatch is drawn, and the label follows it.
        const float square = frame_height(height, heightMode);
        const bool preview = !(flags_ & ImGuiColorEditFlags_NoSmallPreview);
        float intrinsic;

        if (flags_ & ImGuiColorEditFlags_NoInputs) {
            // Without the swatch the label starts at the item's left edge, with no spacing before it.
            const float label = (flags_ & ImGuiColorEditFlags_NoLabel) ? 0.0f : label_width();
            intrinsic = preview ? square + shown_label_extent() : label;
        } else {
            intrinsic = default_field_width() + shown_label_extent();
        }

        return ImVec2{
            fit_measure(intrinsic, width, widthMode),
            fit_measure(square, height, heightMode)
        };
    }

    template <std::size_t N>
    const typename ColorEdit<N>::Value& ColorEdit<N>::value() const {
        return value_;
    }

    template <std::size_t N>
    ColorEdit<N>& ColorEdit<N>::set_value(const Value& value) {
        value_ = value;
        return *this;
    }

    template <std::size_t N>
    ColorEditFlags ColorEdit<N>::flags() const {
        return flags_;
    }

    template <std::size_t N>
    ColorEdit<N>& ColorEdit<N>::set_flags(ColorEditFlags flags) {
        flags_ = flags;
        // NoInputs, NoLabel and NoSmallPreview change the measured size.
        layout().mark_dirty();
        return *this;
    }

    template <std::size_t N>
    ColorEdit<N>& ColorEdit<N>::set_on_change(std::function<void(const Value&)> callback) {
        onChange_ = std::move(callback);
        return *this;
    }
}
