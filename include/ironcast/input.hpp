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
    using InputFlags = ImGuiInputTextFlags;

    // N numeric components edited by typing: Input<1, float>, Input<2, int>, Input<4, float>, ...
    // The value is a plain T when N == 1, otherwise std::array<T, N>. The title is the label, drawn right
    // of the field; the field gets the box width minus the label.
    template <std::size_t N, typename T>
    class Input : public Widget {
        static_assert(N >= 1 && N <= 4, "Input supports 1 to 4 components");
        static_assert(std::same_as<T, int> || std::same_as<T, float>, "Input supports int and float");

    public:
        using Value = std::conditional_t<N == 1, T, std::array<T, N>>;

    private:
        Value value_{};
        T step_ = std::same_as<T, int> ? static_cast<T>(1) : static_cast<T>(0);
        T stepFast_ = std::same_as<T, int> ? static_cast<T>(100) : static_cast<T>(0);
        std::string format_;
        InputFlags flags_ = ImGuiInputTextFlags_None;
        std::function<void(const Value&)> onChange_;

    public:
        Input(const std::string& title = "", Widget* parent = nullptr);

        // True on the frames the value changes.
        bool show() final;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) final;

        [[nodiscard]] const Value& value() const;
        Input& set_value(const Value& value);

        // Amount added by the -/+ buttons, and by the same buttons with Ctrl held.
        // The buttons are hidden when step <= 0. Defaults: 1 and 100 for int, none for float.
        // set_step(step) leaves the fast step unchanged.
        [[nodiscard]] T step() const;
        [[nodiscard]] T step_fast() const;
        Input& set_step(T step);
        Input& set_step(T step, T stepFast);

        // printf-style, applied to each component, e.g. "%.2f" or "%d px". Empty = ImGui default.
        [[nodiscard]] const std::string& format() const;
        Input& set_format(const std::string& format);

        [[nodiscard]] InputFlags flags() const;
        Input& set_flags(InputFlags flags);

        // Called on the next dispatch (see post()) with the new value after the user changes it;
        // set_value() doesn't call it.
        Input& set_on_change(std::function<void(const Value&)> callback);
    };

    template <std::size_t N = 1>
    using InputInt = Input<N, int>;

    template <std::size_t N = 1>
    using InputFloat = Input<N, float>;

    template <std::size_t N, typename T>
    Input<N, T>::Input(const std::string& title, Widget* parent)
    : Widget(title, parent) {
        layout().enable_measure();
    }

    template <std::size_t N, typename T>
    bool Input<N, T>::show() {
        constexpr ImGuiDataType dataType = std::same_as<T, int> ? ImGuiDataType_S32 : ImGuiDataType_Float;

        set_next_field_width(label_extent());

        const std::string& id = title_id();
        const T* step = step_ > static_cast<T>(0) ? &step_ : nullptr;
        const T* stepFast = stepFast_ > static_cast<T>(0) ? &stepFast_ : nullptr;
        const char* format = format_.empty() ? nullptr : format_.c_str();

        T* data;
        if constexpr (N == 1) {
            data = &value_;
        } else {
            data = value_.data();
        }

        const bool pushed = push_frame_height();
        const bool changed = ImGui::InputScalarN(id.c_str(), dataType, data, N, step, stepFast, format, flags_);
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
    ImVec2 Input<N, T>::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        // One frame-height row: the N components share the field width (the -/+ step buttons sit inside the item width), the label follows.
        return measure_field(label_extent(), width, widthMode, height, heightMode);
    }

    template <std::size_t N, typename T>
    const typename Input<N, T>::Value& Input<N, T>::value() const {
        return value_;
    }

    template <std::size_t N, typename T>
    Input<N, T>& Input<N, T>::set_value(const Value& value) {
        value_ = value;
        return *this;
    }

    template <std::size_t N, typename T>
    T Input<N, T>::step() const {
        return step_;
    }

    template <std::size_t N, typename T>
    T Input<N, T>::step_fast() const {
        return stepFast_;
    }

    template <std::size_t N, typename T>
    Input<N, T>& Input<N, T>::set_step(T step) {
        step_ = step;
        return *this;
    }

    template <std::size_t N, typename T>
    Input<N, T>& Input<N, T>::set_step(T step, T stepFast) {
        step_ = step;
        stepFast_ = stepFast;
        return *this;
    }

    template <std::size_t N, typename T>
    const std::string& Input<N, T>::format() const {
        return format_;
    }

    template <std::size_t N, typename T>
    Input<N, T>& Input<N, T>::set_format(const std::string& format) {
        format_ = format;
        return *this;
    }

    template <std::size_t N, typename T>
    InputFlags Input<N, T>::flags() const {
        return flags_;
    }

    template <std::size_t N, typename T>
    Input<N, T>& Input<N, T>::set_flags(InputFlags flags) {
        flags_ = flags;
        return *this;
    }

    template <std::size_t N, typename T>
    Input<N, T>& Input<N, T>::set_on_change(std::function<void(const Value&)> callback) {
        onChange_ = std::move(callback);
        return *this;
    }
}
