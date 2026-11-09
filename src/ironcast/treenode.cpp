#include <imgui_internal.h>
#include "ironcast/assert.hpp"

#include "ironcast/treenode.hpp"
#include "ironcast/ironcast.hpp"

namespace ironcast {
    namespace {
        // ImGui widens a framed header by this much on each side.
        float frame_extend(float windowPadding) {
            return IM_TRUNC(windowPadding * 0.5f);
        }
    }

    TreeNode::Header::Header(TreeNode* parent): Widget("", parent) {
        layout().enable_measure();
    }

    TreeNode& TreeNode::Header::node() {
        // Through the Yoga tree rather than a stored pointer, so it follows a moved node.
        auto* node = static_cast<TreeNode*>(parent());
        IRONCAST_DEBUG_ASSERT(node != nullptr, "TreeNode::Header has no TreeNode");
        return *node;
    }

    bool TreeNode::Header::show() {
        TreeNode& owner = node();
        const ImGuiTreeNodeFlags flags = owner.flags_;
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        // ImGui spans the frame (and the SpanAvailWidth / SpanFullWidth hit box) over the window's
        // work rect; narrow it to this box. A framed header reaches out by frame_extend on both
        // sides, so it starts that much further in.
        const float extend = (flags & ImGuiTreeNodeFlags_Framed) ? frame_extend(window->WindowPadding.x) : 0.0f;
        const ImVec2 start = ImGui::GetCursorScreenPos();
        const ImRect workRect = window->WorkRect;
        window->WorkRect.Min.x = start.x + extend;
        window->WorkRect.Max.x = start.x + width() - extend;
        ImGui::SetCursorScreenPos(ImVec2{start.x + extend, start.y});

        // open_ is the source of truth: ImGui is told every frame, so set_open() needs nothing else.
        ImGui::SetNextItemOpen(owner.open_);
        const bool open = ImGui::TreeNodeEx(owner.title_id().c_str(), flags);
        window->WorkRect = workRect;
        // ImGui pushes whenever it reports open, also on the click that opens the node (before
        // open_ follows) and always for a leaf.
        pushed = open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen);

        // A leaf always reports open; it has no state to change.
        if ((flags & ImGuiTreeNodeFlags_Leaf) || open == owner.open_) {
            return false;
        }

        // Deferred: this frame's boxes are already laid out.
        owner.post([open](auto& self) {
            self.set_open(open);
        });
        return true;
    }

    ImVec2 TreeNode::Header::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        TreeNode& owner = node();
        measuredTitle = owner.title_id();

        // As TreeNodeBehavior lays it out: arrow, padding on both sides of it, label. A frame adds
        // padding before and after the label, and reaches out by frame_extend on each side.
        const ImGuiStyle& style = ImGui::GetStyle();
        const bool framed = owner.flags_ & ImGuiTreeNodeFlags_Framed;
        const bool framePadding = framed || (owner.flags_ & ImGuiTreeNodeFlags_FramePadding);
        const ImVec2 label = ImGui::CalcTextSize(measuredTitle.c_str(), nullptr, true);

        float intrinsicWidth = ImGui::GetFontSize() + label.x + style.FramePadding.x * 2.0f;
        if (framed) {
            intrinsicWidth += style.FramePadding.x * 2.0f + frame_extend(style.WindowPadding.x) * 2.0f;
        }
        // An unframed node starts a line, where ImGui adds no vertical padding.
        const float intrinsicHeight = label.y + (framePadding ? style.FramePadding.y * 2.0f : 0.0f);

        return ImVec2{
            fit_measure(intrinsicWidth, width, widthMode),
            fit_measure(intrinsicHeight, height, heightMode)
        };
    }

    TreeNode::TreeNode(const std::string& title, Widget* parent): Widget(title, parent) {
        column();
    }

    void TreeNode::display_children() {
        for (size_t i = 1; i < layout().child_count(); ++i) {
            Widget* child = layout().child(i)->owner();
            IRONCAST_DEBUG_ASSERT(child != nullptr, "TreeNode child Layout has no owner", i);
            child->display(open_ ? YGDisplayFlex : YGDisplayNone);
        }
    }

    bool TreeNode::is_open() const {
        return open_;
    }

    TreeNode& TreeNode::set_open(bool open) {
        open_ = open;
        display_children();
        return *this;
    }

    TreeNode& TreeNode::set_flags(ImGuiTreeNodeFlags flags) {
        flags_ = flags;
        header.layout().mark_dirty();
        return *this;
    }

    ImGuiTreeNodeFlags TreeNode::flags() const {
        return flags_;
    }

    bool TreeNode::show() {
        // IndentSpacing as this node's style has it; the header takes the padding back with a
        // negative margin, so only the children are indented. Applies from the next layout.
        const float indent = ImGui::GetStyle().IndentSpacing;
        if (layout_padding(YGEdgeLeft) != indent) {
            padding(indent, YGEdgeLeft);
            header.margin(-indent, YGEdgeLeft);
        }

        if (header.measuredTitle != title_id()) {
            header.layout().mark_dirty();
        }

        // Children added while closed are displayed; hide them before the next layout.
        if (!open_) {
            display_children();
        }

        // The header opens an ImGui tree scope; the children are drawn inside it, and TreePop
        // draws the vertical tree line down to them. Indent() from TreePush has no effect: every
        // widget is placed at its Yoga box.
        header.pushed = false;
        const bool changed = Widget::show();
        if (header.pushed) {
            ImGui::TreePop();
        }

        return changed;
    }
}
