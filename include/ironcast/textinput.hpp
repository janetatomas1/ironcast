
#pragma once

#include <string>
#include <functional>

#include "ironcast/widget.hpp"

namespace ironcast {
    using TextInputFlags = ImGuiInputTextFlags;

    // A text field with the title as its label, right of the field. Single line by default, one
    // frame high; set_multiline(true) makes it a multiline editor filling its Yoga box (give it a
    // height).
    class TextInput : public Widget {
        std::string value_;
        std::string hint_;
        std::function<void(const std::string&)> onChange_;
        TextInputFlags flags_ = ImGuiInputTextFlags_None;
        bool multiline_ = false;

    public:
        TextInput(const std::string& title = "", Widget* parent = nullptr);

        // True on the frames the text changes.
        bool show() final;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) final;

        [[nodiscard]] const std::string& value() const;
        TextInput& set_value(const std::string& value);

        // Grey text shown while the field is empty. Empty = none.
        [[nodiscard]] const std::string& hint() const;
        TextInput& set_hint(const std::string& hint);

        [[nodiscard]] TextInputFlags flags() const;
        TextInput& set_flags(TextInputFlags flags);

        // Multiline ignores hint() - InputTextMultiline has no hinted variant in ImGui.
        [[nodiscard]] bool multiline() const;
        TextInput& set_multiline(bool multiline);

        // Called on the next dispatch (see post()) with the new value after the user changes it;
        // set_value() doesn't call it.
        TextInput& set_on_change(std::function<void(const std::string&)> callback);
    };
}
