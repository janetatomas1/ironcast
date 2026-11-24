
#include "ironcast/checkbox.hpp"
#include "ironcast/ironcast.hpp"

namespace ironcast {
    Checkbox::Checkbox(
        std::string title,
        fu2::unique_function<void(bool)> callback,
        Widget* parent
    ): Widget(title, parent),
    callback_(std::move(callback)) {
        layout().enable_measure();
    }

    bool Checkbox::show() {
        bool changed = ImGui::Checkbox(title_id().c_str(), &value_);

        if (changed && callback_) {
            post([value = value_](auto& self) {
                if (self.callback_) {
                    self.callback_(value);
                }
            });
        }

        return changed;
    }

    ImVec2 Checkbox::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        // Same as ImGui::Checkbox: a frame-height square, then the visible label after ItemInnerSpacing.
        const ImVec2 label = ImGui::CalcTextSize(title_id().c_str(), nullptr, true);
        const float square = ImGui::GetFrameHeight();

        return ImVec2{
            fit_measure(square + label_extent(), width, widthMode),
            fit_measure(label.y + ImGui::GetStyle().FramePadding.y * 2.0f, height, heightMode)
        };
    }

    bool Checkbox::value() const {
        return value_;
    }

    Checkbox& Checkbox::set_value(bool value) {
        value_ = value;
        return *this;
    }

    Checkbox& Checkbox::set_callback(fu2::unique_function<void(bool)> callback) {
        callback_ = std::move(callback);
        return *this;
    }

    const fu2::unique_function<void(bool)>& Checkbox::callback() const {
        return callback_;
    }
}
