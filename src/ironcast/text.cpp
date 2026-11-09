
#include "ironcast/text.hpp"

namespace ironcast {
    Text::Text(const std::string& text, Widget* parent)
    : Widget(text, parent) {
        layout().enable_measure();
    }

    bool Text::show() {
        // Wrap at the widget's own width when layout gave it one, otherwise at the window edge.
        const float wrapPos = width() > 0.0f ? ImGui::GetCursorPosX() + width() : 0.0f;

        ImGui::PushTextWrapPos(wrapPos);
        ImGui::TextUnformatted(title().c_str());
        ImGui::PopTextWrapPos();

        return false;
    }

    ImVec2 Text::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        // Height depends on width: with a width constraint the text wraps there, as show() does.
        // TextUnformatted draws "##" literally, so nothing is hidden.
        const bool wrap = widthMode != YGMeasureModeUndefined && width > 0.0f;
        const ImVec2 text = ImGui::CalcTextSize(title().c_str(), nullptr, false, wrap ? width : -1.0f);

        return ImVec2{
            fit_measure(text.x, width, widthMode),
            fit_measure(text.y, height, heightMode)
        };
    }
}
