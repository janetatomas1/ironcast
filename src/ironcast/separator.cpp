#include <algorithm>

#include <imgui_internal.h>

#include "ironcast/separator.hpp"

namespace ironcast {
    namespace {
        // ImGui's thickness for Separator(); SeparatorText uses style.SeparatorTextBorderSize.
        constexpr float lineThickness = 1.0f;
    }

    Separator::Separator(const std::string& title, Widget* parent): Widget(title, parent) {
        layout().enable_measure();
        align_self(YGAlignStretch);
    }

    bool Separator::vertical() const {
        return vertical_;
    }

    Separator& Separator::set_vertical(bool vertical) {
        vertical_ = vertical;
        layout().mark_dirty();
        return *this;
    }

    std::optional<float> Separator::text_align() const {
        return textAlign_;
    }

    Separator& Separator::set_text_align(std::optional<float> align) {
        textAlign_ = align;
        return *this;
    }

    std::optional<float> Separator::left_width() const {
        return leftWidth_;
    }

    Separator& Separator::set_left_width(std::optional<float> width) {
        leftWidth_ = width;
        layout().mark_dirty();
        return *this;
    }

    bool Separator::show() {
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        if (vertical_) {
            // A vertical separator is as tall as ImGui's current line; make that the box height.
            window->DC.CurrLineSize.y = height();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical, lineThickness);
            return false;
        }

        // Horizontal separators run to the right edge of the window's work rect, with no width
        // parameter: narrow it to the box for the call.
        const float workRight = window->WorkRect.Max.x;
        window->WorkRect.Max.x = ImGui::GetCursorScreenPos().x + width();

        if (title().empty()) {
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, lineThickness);
        } else {
            const ImGuiStyle& style = ImGui::GetStyle();
            ImVec2 align = style.SeparatorTextAlign;
            ImVec2 padding = style.SeparatorTextPadding;

            // The left line ends ItemSpacing.x before the title, which starts at padding.x when
            // aligned left.
            if (leftWidth_) {
                align.x = 0.0f;
                padding.x = *leftWidth_ + style.ItemSpacing.x;
            } else if (textAlign_) {
                align.x = *textAlign_;
            }

            ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, align);
            ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextPadding, padding);
            ImGui::SeparatorText(title_id().c_str());
            ImGui::PopStyleVar(2);
        }

        window->WorkRect.Max.x = workRight;
        return false;
    }

    ImVec2 Separator::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        if (vertical_) {
            return ImVec2{
                fit_measure(lineThickness, width, widthMode),
                fit_measure(0.0f, height, heightMode)
            };
        }

        if (title().empty()) {
            return ImVec2{
                fit_measure(0.0f, width, widthMode),
                fit_measure(lineThickness, height, heightMode)
            };
        }

        // SeparatorText's minimum size: title plus padding on both sides, and at least the line.
        const ImGuiStyle& style = ImGui::GetStyle();
        const float paddingX = leftWidth_ ? *leftWidth_ + style.ItemSpacing.x : style.SeparatorTextPadding.x;
        const ImVec2 label = ImGui::CalcTextSize(title_id().c_str(), nullptr, true);

        return ImVec2{
            fit_measure(label.x + paddingX * 2.0f, width, widthMode),
            fit_measure(
                std::max(label.y + style.SeparatorTextPadding.y * 2.0f, style.SeparatorTextBorderSize),
                height,
                heightMode
            )
        };
    }
}
