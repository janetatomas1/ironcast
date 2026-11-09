
#include "ironcast/textinput.hpp"
#include "ironcast/ironcast.hpp"

#include <algorithm>

#include <imgui_stdlib.h>

namespace ironcast {
    TextInput::TextInput(const std::string& title, Widget* parent)
    : Widget(title, parent) {
        layout().enable_measure();
    }

    bool TextInput::show() {
        bool changed;
        if (multiline_) {
            // Explicit frame size, so the label gets the rest of the box. Width must stay positive:
            // ImGui treats a width <= 0 as relative to the window's right edge.
            const ImVec2 frame{std::max(width() - label_extent(), 1.0f), height()};
            changed = ImGui::InputTextMultiline(title_id().c_str(), &value_, frame, flags_);
        } else {
            set_next_field_width(label_extent());

            const bool pushed = push_frame_height();
            if (hint_.empty()) {
                changed = ImGui::InputText(title_id().c_str(), &value_, flags_);
            } else {
                changed = ImGui::InputTextWithHint(title_id().c_str(), hint_.c_str(), &value_, flags_);
            }
            pop_frame_height(pushed);
        }

        if (changed && onChange_) {
            post([value = value_](auto& self) {
                if (self.onChange_) {
                    self.onChange_(value);
                }
            });
        }

        return changed;
    }

    ImVec2 TextInput::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        if (!multiline_) {
            return measure_field(label_extent(), width, widthMode, height, heightMode);
        }

        // ImGui's default multiline frame: 8 lines of text plus frame padding. The label sits to its right.
        const float frameHeight = ImGui::GetFontSize() * 8.0f + ImGui::GetStyle().FramePadding.y * 2.0f;

        return ImVec2{
            fit_measure(default_field_width() + label_extent(), width, widthMode),
            fit_measure(frameHeight, height, heightMode)
        };
    }

    const std::string& TextInput::value() const {
        return value_;
    }

    TextInput& TextInput::set_value(const std::string& value) {
        value_ = value;
        return *this;
    }

    const std::string& TextInput::hint() const {
        return hint_;
    }

    TextInput& TextInput::set_hint(const std::string& hint) {
        hint_ = hint;
        return *this;
    }

    TextInputFlags TextInput::flags() const {
        return flags_;
    }

    TextInput& TextInput::set_flags(TextInputFlags flags) {
        flags_ = flags;
        return *this;
    }

    bool TextInput::multiline() const {
        return multiline_;
    }

    TextInput& TextInput::set_multiline(bool multiline) {
        multiline_ = multiline;
        layout().mark_dirty();
        return *this;
    }

    TextInput& TextInput::set_on_change(std::function<void(const std::string&)> callback) {
        onChange_ = std::move(callback);
        return *this;
    }
}
