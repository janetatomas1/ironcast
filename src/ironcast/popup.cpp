#include <imgui_internal.h>

#include "ironcast/popup.hpp"

namespace ironcast {
    Popup::Popup(const std::string& title, Widget* parent): Widget(title, parent) {
        layout().set_origin(true);
        position_type(YGPositionTypeAbsolute);
    }

    bool Popup::begin(ImGuiWindowFlags flags) {
        return ImGui::BeginPopup(title_id().c_str(), flags);
    }

    Popup& Popup::set_window_flags(ImGuiWindowFlags flags) {
        flags_ = flags;
        return *this;
    }

    ImGuiWindowFlags Popup::flags() const {
        return flags_;
    }

    void Popup::open() {
        open_ = true;
        openRequested_ = true;
    }

    void Popup::close() {
        open_ = false;
        openRequested_ = false;
    }

    bool Popup::is_open() const {
        return open_;
    }

    bool Popup::show() {
        // OpenPopup and the Begin call must see the same ID stack, so both happen here.
        if (openRequested_) {
            if (!ImGui::IsPopupOpen(title_id().c_str())) {
                ImGui::OpenPopup(title_id().c_str());
            }
            openRequested_ = false;
        }

        // render() put the cursor at this widget's box; the popup is a separate window,
        // so its position is given in screen space.
        ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos());
        ImGui::SetNextWindowSize(size());
        // No item follows that cursor move in the parent window (the popup takes no space there),
        // which ImGui would report as using SetCursorPos to extend the window's boundaries.
        ImGui::GetCurrentWindow()->DC.IsSetPos = false;

        // Yoga padding is the only inset: SetCursorPos ignores WindowPadding, but ImGui would
        // still shrink the clip rect by it.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
        const bool visible = begin(flags_);
        ImGui::PopStyleVar();

        if (!visible) {
            // Picks up closes ImGui did on its own (click outside, Escape), so they don't reopen.
            // Only ever cleared here: a pending close() must survive until CloseCurrentPopup below.
            open_ = false;
            return false;
        }

        // Local (0, 0) is the window's top-left corner, under the title bar if there is one:
        // keep children below it.
        const ImGuiWindow* window = ImGui::GetCurrentWindow();
        const float borderSize = ImGui::GetStyle().PopupBorderSize;
        border(borderSize, YGEdgeAll);
        border(borderSize + window->DecoOuterSizeY1, YGEdgeTop);

        const bool changed = Widget::show();

        // Re-read: a child may have called close() while drawing.
        if (!open_) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
        return changed;
    }
}
