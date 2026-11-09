#include <imgui_internal.h>

#include "ironcast/childwindow.hpp"

namespace ironcast {
    ChildWindow::ChildWindow(const std::string& title, Widget* parent): Widget(title, parent) {
        layout().set_origin(true);
        overflow(YGOverflowScroll);
    }

    ChildWindow& ChildWindow::set_child_flags(ImGuiChildFlags flags) {
        childFlags_ = flags;
        return *this;
    }

    ImGuiChildFlags ChildWindow::child_flags() const {
        return childFlags_;
    }

    ChildWindow& ChildWindow::set_window_flags(ImGuiWindowFlags flags) {
        windowFlags_ = flags;
        return *this;
    }

    ImGuiWindowFlags ChildWindow::window_flags() const {
        return windowFlags_;
    }

    bool ChildWindow::show() {
        const ImGuiStyle& style = ImGui::GetStyle();
        const float borderSize = (childFlags_ & ImGuiChildFlags_Borders) ? style.ChildBorderSize : 0.0f;

        // Yoga padding is the only inset: SetCursorPos ignores WindowPadding, but ImGui would
        // still shrink the clip rect by it.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
        // id(), not the title: renaming must not reset the scroll position.
        const bool visible = ImGui::BeginChild(static_cast<ImGuiID>(id()), size(), childFlags_, windowFlags_);
        ImGui::PopStyleVar();

        const ImGuiWindow* window = ImGui::GetCurrentWindow();
        border(borderSize, YGEdgeAll);
        border(borderSize + (window->ScrollbarY ? style.ScrollbarSize : 0.0f), YGEdgeRight);
        border(borderSize + (window->ScrollbarX ? style.ScrollbarSize : 0.0f), YGEdgeBottom);

        bool changed = false;
        if (visible) {
            changed = Widget::show();

            // Scroll range from what was drawn here: ImGui sizes the content by the items' far
            // corner, so children drawn in their own window (popups) don't count. It doesn't know
            // about the trailing padding and border line, so they are added to that corner.
            ImGuiWindow* child = ImGui::GetCurrentWindow();
            child->DC.CursorMaxPos.x += layout_padding(YGEdgeRight) + borderSize;
            child->DC.CursorMaxPos.y += layout_padding(YGEdgeBottom) + borderSize;
        }

        // Unlike most Begin/End pairs, EndChild is required even when BeginChild returns false.
        ImGui::EndChild();
        return changed;
    }
}
