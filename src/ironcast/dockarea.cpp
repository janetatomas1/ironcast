#include <algorithm>

#include <imgui_internal.h>
#include "ironcast/assert.hpp"

#include "ironcast/dockarea.hpp"
#include "ironcast/ironcast.hpp"

namespace ironcast {
    DockArea::DockArea(Widget* parent): Widget("", parent) {}

    void DockArea::attach_pane(std::unique_ptr<Pane> pane) {
        post([pane = std::move(pane)](auto& self) mutable {
            auto& ref = self.adopt(std::move(pane));
            ref.set_parent(&self);
            // Panes added before the window starts are initialized with it.
            if (self.in_window_tree()) {
                ref.init();
            }
        });
    }

    void DockArea::remove_pane(size_t index) {
        post([index](auto& self) {
            IRONCAST_DEBUG_ASSERT(index < self.count(), "DockArea::remove_pane index out of range", index, self.count());
            // The pane's window is no longer submitted, so ImGui drops it from its node.
            self.remove(index);
        });
    }

    DockArea& DockArea::set_flags(ImGuiDockNodeFlags flags) {
        flags_ = flags;
        return *this;
    }

    ImGuiDockNodeFlags DockArea::flags() const {
        return flags_;
    }

    void DockArea::build() {
        const ImGuiID dockId = static_cast<ImGuiID>(id());

        ImGui::DockBuilderRemoveNode(dockId);
        ImGui::DockBuilderAddNode(dockId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockId, size());

        // Each sided pane is split off what is left, in add order; the rest is the middle. The
        // pane's ratio is of the whole area, so it is converted to a share of what is left.
        ImGuiID middle = dockId;
        ImVec2 remaining = size();
        for (Pane& pane : items()) {
            if (pane.dockSide_ != ImGuiDir_None) {
                const bool horizontal = pane.dockSide_ == ImGuiDir_Left || pane.dockSide_ == ImGuiDir_Right;
                float& remainingSize = horizontal ? remaining.x : remaining.y;
                const float wanted = pane.ratio_ * (horizontal ? width() : height());
                const float ratio = remainingSize > 0.0f ? std::min(wanted / remainingSize, 1.0f) : 0.0f;
                remainingSize = std::max(remainingSize - wanted, 0.0f);

                ImGuiID side = 0;
                ImGui::DockBuilderSplitNode(middle, pane.dockSide_, ratio, &side, &middle);
                pane.dockNode_ = side;
                pane.placed_ = true;
            }
        }

        for (Pane& pane : items()) {
            if (!pane.placed_) {
                pane.dockNode_ = middle;
                pane.placed_ = true;
            }
        }

        ImGui::DockBuilderFinish(dockId);
        built_ = true;
    }

    bool DockArea::show() {
        // No box yet: nodes built now would split nothing, and DockSpace() reads a zero size as
        // "fill the window".
        if (width() <= 0.0f || height() <= 0.0f) {
            return false;
        }

        const ImGuiID dockId = static_cast<ImGuiID>(id());
        if (!built_ || ImGui::DockBuilderGetNode(dockId) == nullptr) {
            build();
        }

        // The dock space fills this widget's box; render() put the cursor at its corner.
        ImGui::DockSpace(dockId, size(), flags_);

        // Panes added after the first layout go to the middle as tabs.
        const ImGuiDockNode* central = ImGui::DockBuilderGetCentralNode(dockId);
        const ImGuiID middle = central != nullptr ? central->ID : dockId;

        bool changed = false;
        for (Pane& pane : items()) {
            // A closed pane calls no Begin, so a dock id set for it would go to the next pane's
            // window. Its node waits until it is opened.
            if (!pane.is_open()) {
                continue;
            }

            if (!pane.placed_) {
                pane.dockNode_ = middle;
                pane.placed_ = true;
            }

            if (pane.dockNode_ != 0) {
                ImGui::SetNextWindowDockID(pane.dockNode_);
                pane.dockNode_ = 0;
            }

            changed |= pane.draw();
        }

        return changed;
    }
}
