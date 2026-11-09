#pragma once

#include <string>

#include "ironcast/widget.hpp"

namespace ironcast {
    // A collapsible node: an ImGui tree node header labelled with the title, then the node's
    // children, indented by the style's IndentSpacing. Children are any widgets, added by passing
    // the node as parent; nest TreeNodes to build a tree. The header is the first Yoga child.
    // While closed, the children get display(YGDisplayNone), so Yoga gives them no space.
    // The indent is the node's left padding, kept in sync with IndentSpacing while drawing.
    // The children are drawn inside the ImGui tree scope, so ImGui's tree lines connect a node to
    // its child nodes. Set DrawLinesFull or DrawLinesToNodes on the child nodes too (each draws its
    // own connector), or once for all nodes in style.TreeLinesFlags.
    class TreeNode : public Widget {
        struct Header: Widget {
            // The title measure() last saw; a rename re-measures the header.
            std::string measuredTitle;
            // TreeNodeEx returned true this frame, so ImGui pushed a tree scope that the node pops
            // after its children.
            bool pushed = false;

            Header(TreeNode* parent);
            [[nodiscard]] TreeNode& node();
            bool show() override;
            ImVec2 measure(
                float width,
                YGMeasureMode widthMode,
                float height,
                YGMeasureMode heightMode
            ) override;
        };

        Header header = Header(this);
        bool open_ = false;
        ImGuiTreeNodeFlags flags_ = ImGuiTreeNodeFlags_None;

        // Shows or hides every child after the header, to match open_.
        void display_children();

    public:
        TreeNode(const std::string& title = "", Widget* parent = nullptr);

        [[nodiscard]] bool is_open() const;
        // Applies now; a click on the header applies on the next dispatch.
        TreeNode& set_open(bool open);

        // ImGuiTreeNodeFlags, e.g. Framed, Leaf, Bullet, SpanAvailWidth, DrawLinesToNodes. The open
        // state is set_open()'s, so DefaultOpen has no effect. NoTreePushOnOpen turns tree lines off.
        TreeNode& set_flags(ImGuiTreeNodeFlags flags);
        [[nodiscard]] ImGuiTreeNodeFlags flags() const;

        // True when the header was clicked open or closed.
        bool show() override;
    };
}
