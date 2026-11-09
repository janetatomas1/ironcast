
#include "ironcast/assert.hpp"

#include "ironcast/radiobutton.hpp"
#include "ironcast/ironcast.hpp"

namespace ironcast {
    RadioButton::RadioButton(const std::string& title, int* groupValue, int value, Widget* parent)
    : Widget(title, parent), groupValue_(groupValue), value_(value) {
        IRONCAST_DEBUG_ASSERT(groupValue_ != nullptr, "RadioButton requires a non-null groupValue pointer", title);
        layout().enable_measure();
    }

    RadioButton::RadioButton(const std::string& title, int* groupValue, int value,
                              std::function<void()> onSelect, Widget* parent)
    : Widget(title, parent), groupValue_(groupValue), value_(value), onSelect_(std::move(onSelect)) {
        IRONCAST_DEBUG_ASSERT(groupValue_ != nullptr, "RadioButton requires a non-null groupValue pointer", title);
        layout().enable_measure();
    }

    bool RadioButton::show() {
        IRONCAST_DEBUG_ASSERT(groupValue_ != nullptr, "RadioButton::show called with no groupValue bound");

        bool wasSelected = selected();
        bool clicked = ImGui::RadioButton(title_id().c_str(), groupValue_, value_);

        if (clicked && !wasSelected && onSelect_) {
            post([](auto& self) {
                if (self.onSelect_) {
                    self.onSelect_();
                }
            });
        }

        return clicked;
    }

    ImVec2 RadioButton::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        // Same as ImGui::RadioButton: a frame-height circle, then the visible label after ItemInnerSpacing.
        const ImVec2 label = ImGui::CalcTextSize(title_id().c_str(), nullptr, true);
        const float circle = ImGui::GetFrameHeight();

        return ImVec2{
            fit_measure(circle + label_extent(), width, widthMode),
            fit_measure(label.y + ImGui::GetStyle().FramePadding.y * 2.0f, height, heightMode)
        };
    }

    bool RadioButton::selected() const {
        IRONCAST_DEBUG_ASSERT(groupValue_ != nullptr, "RadioButton::selected called with no groupValue bound");
        return *groupValue_ == value_;
    }

    void RadioButton::select() {
        IRONCAST_DEBUG_ASSERT(groupValue_ != nullptr, "RadioButton::select called with no groupValue bound");
        *groupValue_ = value_;
    }

    RadioButton& RadioButton::set_on_select(std::function<void()> callback) {
        onSelect_ = std::move(callback);
        return *this;
    }
}