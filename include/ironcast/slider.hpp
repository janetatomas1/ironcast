#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>

#include "ironcast/widget.hpp"
#include "ironcast/ironcast.hpp"

namespace ironcast {
    using SliderFlags = ImGuiSliderFlags;

    // N numeric components edited with sliders: Slider<1, float>, Slider<2, int>, Slider<4, float>, ...
    // The value is a plain T when N == 1, otherwise std::array<T, N>. The title is the label, drawn right
    // of the field; the field gets the box width minus the label.
    template <std::size_t N, typename T>
    class Slider : public Widget {
        static_assert(N >= 1 && N <= 4, "Slider supports 1 to 4 components");
        static_assert(std::same_as<T, int> || std::same_as<T, float>, "Slider supports int and float");

    public:
        using Value = std::conditional_t<N == 1, T, std::array<T, N>>;

    private:
        Value value_{};
        T min_ = static_cast<T>(0);
        T max_ = std::same_as<T, int> ? static_cast<T>(100) : static_cast<T>(1);
        std::string format_;
        SliderFlags flags_ = ImGuiSliderFlags_None;
        std::function<void(const Value&)> onChange_;

    public:
        Slider(const std::string& title = "", Widget* parent = nullptr);

        // True on the frames the value changes.
        bool show() final;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) final;

        [[nodiscard]] const Value& value() const;
        Slider& set_value(const Value& value);

        // The range applies to every component. Defaults: 0..100 for int, 0..1 for float.
        [[nodiscard]] T min() const;
        [[nodiscard]] T max() const;
        Slider& set_range(T min, T max);

        // printf-style, applied to each component, e.g. "%.2f" or "%d px". Empty = ImGui default.
        [[nodiscard]] const std::string& format() const;
        Slider& set_format(const std::string& format);

        [[nodiscard]] SliderFlags flags() const;
        Slider& set_flags(SliderFlags flags);

        // Called on the next dispatch (see post()) with the new value after the user changes it;
        // set_value() doesn't call it.
        Slider& set_on_change(std::function<void(const Value&)> callback);
    };

    template <std::size_t N = 1>
    using SliderInt = Slider<N, int>;

    template <std::size_t N = 1>
    using SliderFloat = Slider<N, float>;

    template <std::size_t N, typename T>
    Slider<N, T>::Slider(const std::string& title, Widget* parent)
    : Widget(title, parent) {
        layout().enable_measure();
    }

    template <std::size_t N, typename T>
    bool Slider<N, T>::show() {
        constexpr ImGuiDataType dataType = std::same_as<T, int> ? ImGuiDataType_S32 : ImGuiDataType_Float;

        set_next_field_width(label_extent());

        const std::string& id = title_id();
        const char* format = format_.empty() ? nullptr : format_.c_str();

        T* data;
        if constexpr (N == 1) {
            data = &value_;
        } else {
            data = value_.data();
        }

        const bool pushed = push_frame_height();
        const bool changed = ImGui::SliderScalarN(id.c_str(), dataType, data, N, &min_, &max_, format, flags_);
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

    template <std::size_t N, typename T>
    ImVec2 Slider<N, T>::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        // One frame-height row: the N components share the field width, the label follows.
        return measure_field(label_extent(), width, widthMode, height, heightMode);
    }

    template <std::size_t N, typename T>
    const typename Slider<N, T>::Value& Slider<N, T>::value() const {
        return value_;
    }

    template <std::size_t N, typename T>
    Slider<N, T>& Slider<N, T>::set_value(const Value& value) {
        value_ = value;
        return *this;
    }

    template <std::size_t N, typename T>
    T Slider<N, T>::min() const {
        return min_;
    }

    template <std::size_t N, typename T>
    T Slider<N, T>::max() const {
        return max_;
    }

    template <std::size_t N, typename T>
    Slider<N, T>& Slider<N, T>::set_range(T min, T max) {
        min_ = min;
        max_ = max;
        return *this;
    }

    template <std::size_t N, typename T>
    const std::string& Slider<N, T>::format() const {
        return format_;
    }

    template <std::size_t N, typename T>
    Slider<N, T>& Slider<N, T>::set_format(const std::string& format) {
        format_ = format;
        return *this;
    }

    template <std::size_t N, typename T>
    SliderFlags Slider<N, T>::flags() const {
        return flags_;
    }

    template <std::size_t N, typename T>
    Slider<N, T>& Slider<N, T>::set_flags(SliderFlags flags) {
        flags_ = flags;
        return *this;
    }

    template <std::size_t N, typename T>
    Slider<N, T>& Slider<N, T>::set_on_change(std::function<void(const Value&)> callback) {
        onChange_ = std::move(callback);
        return *this;
    }
}
