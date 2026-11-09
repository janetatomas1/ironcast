#pragma once

#include "ironcast/childwindow.hpp"

namespace ironcast {
    // A scrolling child window for a tree of TreeNodes: add the top-level nodes with the view as
    // their parent. expand_all() / collapse_all() reach every TreeNode below it, at any depth,
    // including nodes nested inside other widgets.
    class TreeView : public ChildWindow {
        void set_all_open(Widget& widget, bool open);

    public:
        TreeView(const std::string& title = "", Widget* parent = nullptr);

        // Leaves are skipped.
        TreeView& expand_all();
        TreeView& collapse_all();
    };
}
