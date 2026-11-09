
#include "ironcast/button.hpp"
#include "ironcast/ironcast.hpp"

#include <algorithm>

namespace ironcast {
    Button::Button(const std::string &title, Widget *parent)
    : Widget(title, parent) {
        layout().enable_measure();
    }

    Button::Button(const std::string &title, fu2::unique_function<void()> callback, Widget *parent)
    : Widget(title, parent), callback_(std::move(callback)) {
        layout().enable_measure();
    }

    bool Button::show() {
        if (ImGui::Button(title_id().c_str(), size())){
            if (callback_) {
                post([](auto& self) {
                    if (self.callback_) {
                        self.callback_();
                    }
                });
            }

            return true;
        }

        return false;
    }

    ImVec2 Button::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        // Same as ImGui::Button's auto size: visible label (text after "##" hidden) plus frame padding.
        const ImGuiStyle& style = ImGui::GetStyle();
        const ImVec2 label = ImGui::CalcTextSize(title_id().c_str(), nullptr, true);

        return ImVec2{
            fit_measure(label.x + style.FramePadding.x * 2.0f, width, widthMode),
            fit_measure(label.y + style.FramePadding.y * 2.0f, height, heightMode)
        };
    }

    void Button::set_callback(fu2::unique_function<void()> callback) {
        callback_ = std::move(callback);
    }
}
