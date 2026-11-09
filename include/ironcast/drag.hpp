#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <type_traits>

#include "ironcast/widget.hpp"
#include "ironcast/ironcast.hpp"

namespace ironcast {
    using DragFlags = ImGuiSliderFlags;

    // N numeric components edited by dragging: Drag<1, float>, Drag<2, int>, Drag<4, float>, ...
    // The value is a plain T when N == 1, otherwise std::array<T, N>. The title is the label, drawn right
    // of the field; the field gets the box width minus the label.
    template <std::size_t N, typename T>
    class Drag : public Widget {
        static_assert(N >= 1 && N <= 4, "Drag supports 1 to 4 components");
        static_assert(std::same_as<T, int> || std::same_as<T, float>, "Drag supports int and float");

    public:
        using Value = std::conditional_t<N == 1, T, std::array<T, N>>;

    private:
        Value value_{};
        float speed_ = std::same_as<T, float> ? 0.01f : 1.0f;
        std::optional<T> min_;
        std::optional<T> max_;
        std::string format_;
        DragFlags flags_ = ImGuiSliderFlags_None;
        std::function<void(const Value&)> onChange_;

    public:
        Drag(const std::string& title = "", Widget* parent = nullptr);

        // True on the frames the value changes.
        bool show() final;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) final;

        [[nodiscard]] const Value& value() const;
        Drag& set_value(const Value& value);

        // Value change per pixel dragged.
        [[nodiscard]] float speed() const;
        Drag& set_speed(float speed);

        // The range applies to every component. Without one the value is unbounded.
        [[nodiscard]] std::optional<T> min() const;
        [[nodiscard]] std::optional<T> max() const;
        Drag& set_range(T min, T max);
        Drag& clear_range();

        // printf-style, applied to each component, e.g. "%.2f" or "%d px". Empty = ImGui default.
        [[nodiscard]] const std::string& format() const;
        Drag& set_format(const std::string& format);

        [[nodiscard]] DragFlags flags() const;
        Drag& set_flags(DragFlags flags);

        // Called on the next dispatch (see post()) with the new value after the user changes it;
        // set_value() doesn't call it.
        Drag& set_on_change(std::function<void(const Value&)> callback);
    };

    template <std::size_t N = 1>
    using DragInt = Drag<N, int>;

    template <std::size_t N = 1>
    using DragFloat = Drag<N, float>;

    template <std::size_t N, typename T>
    Drag<N, T>::Drag(const std::string& title, Widget* parent)
    : Widget(title, parent) {
        layout().enable_measure();
    }

    template <std::size_t N, typename T>
    bool Drag<N, T>::show() {
        constexpr ImGuiDataType dataType = std::same_as<T, int> ? ImGuiDataType_S32 : ImGuiDataType_Float;

        set_next_field_width(label_extent());

        const std::string& id = title_id();
        const T* min = min_ ? &*min_ : nullptr;
        const T* max = max_ ? &*max_ : nullptr;
        const char* format = format_.empty() ? nullptr : format_.c_str();

        T* data;
        if constexpr (N == 1) {
            data = &value_;
        } else {
            data = value_.data();
        }

        const bool pushed = push_frame_height();
        const bool changed = ImGui::DragScalarN(id.c_str(), dataType, data, N, speed_, min, max, format, flags_);
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
    ImVec2 Drag<N, T>::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        // One frame-height row: the N components share the field width, the label follows.
        return measure_field(label_extent(), width, widthMode, height, heightMode);
    }

    template <std::size_t N, typename T>
    const typename Drag<N, T>::Value& Drag<N, T>::value() const {
        return value_;
    }

    template <std::size_t N, typename T>
    Drag<N, T>& Drag<N, T>::set_value(const Value& value) {
        value_ = value;
        return *this;
    }

    template <std::size_t N, typename T>
    float Drag<N, T>::speed() const {
        return speed_;
    }

    template <std::size_t N, typename T>
    Drag<N, T>& Drag<N, T>::set_speed(float speed) {
        speed_ = speed;
        return *this;
    }

    template <std::size_t N, typename T>
    std::optional<T> Drag<N, T>::min() const {
        return min_;
    }

    template <std::size_t N, typename T>
    std::optional<T> Drag<N, T>::max() const {
        return max_;
    }

    template <std::size_t N, typename T>
    Drag<N, T>& Drag<N, T>::set_range(T min, T max) {
        min_ = min;
        max_ = max;
        return *this;
    }

    template <std::size_t N, typename T>
    Drag<N, T>& Drag<N, T>::clear_range() {
        min_.reset();
        max_.reset();
        return *this;
    }

    template <std::size_t N, typename T>
    const std::string& Drag<N, T>::format() const {
        return format_;
    }

    template <std::size_t N, typename T>
    Drag<N, T>& Drag<N, T>::set_format(const std::string& format) {
        format_ = format;
        return *this;
    }

    template <std::size_t N, typename T>
    DragFlags Drag<N, T>::flags() const {
        return flags_;
    }

    template <std::size_t N, typename T>
    Drag<N, T>& Drag<N, T>::set_flags(DragFlags flags) {
        flags_ = flags;
        return *this;
    }

    template <std::size_t N, typename T>
    Drag<N, T>& Drag<N, T>::set_on_change(std::function<void(const Value&)> callback) {
        onChange_ = std::move(callback);
        return *this;
    }
}
