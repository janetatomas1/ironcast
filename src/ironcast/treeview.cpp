#include "ironcast/assert.hpp"

#include "ironcast/treenode.hpp"
#include "ironcast/treeview.hpp"

namespace ironcast {
    TreeView::TreeView(const std::string& title, Widget* parent): ChildWindow(title, parent) {
        set_child_flags(ImGuiChildFlags_Borders);
    }

    void TreeView::set_all_open(Widget& widget, bool open) {
        for (size_t i = 0; i < widget.layout().child_count(); ++i) {
            Widget* child = widget.layout().child(i)->owner();
            IRONCAST_DEBUG_ASSERT(child != nullptr, "TreeView: child Layout has no owner", i);

            auto* node = dynamic_cast<TreeNode*>(child);
            if (node != nullptr && !(node->flags() & ImGuiTreeNodeFlags_Leaf)) {
                node->set_open(open);
            }
            set_all_open(*child, open);
        }
    }

    TreeView& TreeView::expand_all() {
        set_all_open(*this, true);
        return *this;
    }

    TreeView& TreeView::collapse_all() {
        set_all_open(*this, false);
        return *this;
    }
}
