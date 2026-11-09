#include <algorithm>

#include <imgui_internal.h>
#include "ironcast/assert.hpp"

#include "ironcast/pane.hpp"
#include "ironcast/ironcast.hpp"

namespace ironcast {
    Pane::Pane(const std::string& title, Widget* parent): Widget(title, parent) {
        layout().set_origin(true);
        position_type(YGPositionTypeAbsolute);
    }

    Pane& Pane::set_dock(ImGuiDir side) {
        dockSide_ = side;
        return *this;
    }

    ImGuiDir Pane::dock() const {
        return dockSide_;
    }

    Pane& Pane::set_ratio(float ratio) {
        IRONCAST_DEBUG_ASSERT(ratio > 0.0f && ratio <= 1.0f, "Pane::set_ratio ratio out of (0, 1]", ratio);

        post([ratio](Pane& self) {
            self.ratio_ = ratio;
            // Not laid out yet: build() reads ratio_.
            if (!self.placed_) {
                return;
            }

            // Actions run between frames, with any window's ImGui context current.
            self.window()->backend()->make_current();
            ImGuiWindow* imguiWindow = ImGui::FindWindowByName(self.title_id().c_str());
            if (imguiWindow == nullptr || imguiWindow->DockNode == nullptr || imguiWindow->DockNode->ParentNode == nullptr) {
                return;
            }

            // A locked size is kept exactly on the next layout and the sibling takes the rest, as
            // when the user drags the split.
            ImGuiDockNode* node = imguiWindow->DockNode;
            const ImGuiAxis axis = static_cast<ImGuiAxis>(node->ParentNode->SplitAxis);
            node->Size[axis] = std::max(self.ratio_ * ImGui::DockNodeGetRootNode(node)->Size[axis], 1.0f);
            node->WantLockSizeOnce = true;
        });
        return *this;
    }

    float Pane::ratio() const {
        return ratio_;
    }

    Pane& Pane::set_dock_flags(ImGuiDockNodeFlags flags) {
        dockFlags_ = flags;
        return *this;
    }

    ImGuiDockNodeFlags Pane::dock_flags() const {
        return dockFlags_;
    }

    Pane& Pane::set_window_flags(ImGuiWindowFlags flags) {
        windowFlags_ = flags;
        return *this;
    }

    ImGuiWindowFlags Pane::window_flags() const {
        return windowFlags_;
    }

    Pane& Pane::set_closable(bool closable) {
        closable_ = closable;
        return *this;
    }

    bool Pane::closable() const {
        return closable_;
    }

    void Pane::open() {
        open_ = true;
    }

    void Pane::close() {
        open_ = false;
    }

    bool Pane::is_open() const {
        return open_;
    }

    void Pane::set_collapsed(bool collapsed) {
        post([collapsed](Pane& self) {
            // Actions run between frames, with any window's ImGui context current.
            self.window()->backend()->make_current();
            if (ImGuiWindow* imguiWindow = ImGui::FindWindowByName(self.title_id().c_str())) {
                ImGui::SetWindowCollapsed(imguiWindow, collapsed);
            }
        });
    }

    void Pane::minimize() {
        set_collapsed(true);
    }

    void Pane::restore() {
        set_collapsed(false);
    }

    bool Pane::is_minimized() const {
        return minimized_;
    }

    bool Pane::show() {
        if (!open_) {
            return false;
        }

        if (dockFlags_ != ImGuiDockNodeFlags_None) {
            ImGuiWindowClass windowClass;
            windowClass.DockNodeFlagsOverrideSet = dockFlags_;
            ImGui::SetNextWindowClass(&windowClass);
        }

        // Yoga padding is the only inset, as in Popup and ChildWindow. The ids change every run,
        // so saving the window's settings is pointless.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
        const bool visible = ImGui::Begin(title_id().c_str(), closable_ ? &open_ : nullptr, windowFlags_ | ImGuiWindowFlags_NoSavedSettings);
        ImGui::PopStyleVar();
        minimized_ = ImGui::IsWindowCollapsed();

        bool changed = false;
        if (visible) {
            // The pane's box is the whole window; local (0, 0) is its top-left corner, so the
            // title bar (floating) or tab bar (docked) is kept clear with the top border.
            const ImGuiWindow* window = ImGui::GetCurrentWindow();
            border(window->DecoOuterSizeY1, YGEdgeTop);

            // Sized to its window, so the children are laid out inside it. Yoga sees the new size on
            // the next frame.
            if (window->Size.x != width() || window->Size.y != height()) {
                width(window->Size.x);
                height(window->Size.y);
                Ironcast::current().request_redraw();
            }

            changed = Widget::show();
        }

        // Unlike most Begin/End pairs, End is required even when Begin returns false.
        ImGui::End();
        return changed;
    }
}
