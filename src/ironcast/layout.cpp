
#include "ironcast/assert.hpp"

#include <algorithm>
#include <vector>

#include "ironcast/layout.hpp"
#include "ironcast/widget.hpp"

namespace ironcast {
    namespace {
        // Yoga only calls this from YGNodeCalculateLayout, i.e. inside the frame, so
        // measure() may use ImGui metrics. Resolved through the node context on every
        // call, so it stays valid across Layout/Widget moves.
        YGSize
        measure_trampoline(
            YGNodeConstRef node,
            float width,
            YGMeasureMode widthMode,
            float height,
            YGMeasureMode heightMode
        ) {
            IRONCAST_DEBUG_ASSERT(ImGui::GetCurrentContext() != nullptr, "measure called with no ImGui context");

            auto layout = static_cast<Layout*>(YGNodeGetContext(node));
            IRONCAST_DEBUG_ASSERT(layout != nullptr, "measured Yoga node has no Layout context");
            IRONCAST_DEBUG_ASSERT(layout->owner() != nullptr, "measured Layout has no owner");

            // Layout runs before anything is drawn, so no ancestor's style is pushed yet. Push
            // them root first, as draw() would have, so the widget measures as it is drawn.
            std::vector<Widget*> chain;
            for (Widget* widget = layout->owner(); widget != nullptr; widget = widget->parent()) {
                chain.push_back(widget);
            }

            for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
                (*it)->style().push();
            }

            ImVec2 size = layout->owner()->measure(width, widthMode, height, heightMode);

            for (Widget* widget : chain) {
                widget->style().pop();
            }

            return YGSize{size.x, size.y};
        }
    }

    float fit_measure(float intrinsic, float available, YGMeasureMode mode) {
        switch (mode) {
            case YGMeasureModeExactly: return available;
            case YGMeasureModeAtMost:  return std::min(intrinsic, available);
            default:                   return intrinsic;
        }
    }

    Layout::Layout(): node_(YGNodeNew()) {
        IRONCAST_DEBUG_ASSERT(node_ != nullptr, "YGNodeNew failed");
        YGNodeSetContext(node_, this);
    }

    Layout::~Layout() {
        if (node_ == nullptr) {
            return; // moved-from
        }

        remove_from_parent();

        YGNodeSetContext(node_, nullptr);
        YGNodeFree(node_);
    }

    Layout::Layout(Layout&& other) noexcept
        : node_(std::exchange(other.node_, nullptr))
        , position_(other.position_)
        , size_(other.size_)
        , owner_(other.owner_)
        , origin_(other.origin_) {
        if (node_ != nullptr) {
            YGNodeSetContext(node_, this);
        }
    }

    Layout& Layout::operator=(Layout&& other) noexcept {
        if (this != &other) {
            if (node_ != nullptr) {
                remove_from_parent();
                YGNodeSetContext(node_, nullptr);
                YGNodeFree(node_);
            }

            node_ = std::exchange(other.node_, nullptr);
            position_ = other.position_;
            size_ = other.size_;
            owner_ = other.owner_;
            origin_ = other.origin_;

            if (node_ != nullptr) {
                YGNodeSetContext(node_, this);
            }
        }

        return *this;
    }

    YGNodeRef Layout::node() {
        IRONCAST_DEBUG_ASSERT(node_ != nullptr, "Layout::node called on moved-from Layout");
        return node_;
    }

    void Layout::set_owner(Widget* owner) {
        owner_ = owner;
    }

    Widget* Layout::owner() const {
        return owner_;
    }

    void Layout::enable_measure() {
        IRONCAST_DEBUG_ASSERT(node_ != nullptr, "enable_measure called on moved-from Layout");
        IRONCAST_DEBUG_ASSERT(YGNodeGetChildCount(node_) == 0, "enable_measure: measured nodes must be leaves");

        YGNodeSetMeasureFunc(node_, measure_trampoline);
    }

    void Layout::mark_dirty() {
        IRONCAST_DEBUG_ASSERT(node_ != nullptr, "mark_dirty called on moved-from Layout");

        if (YGNodeHasMeasureFunc(node_)) {
            YGNodeMarkDirty(node_);
        }

        // Entries drawn outside Yoga (menus, table cells) are measured by their owner instead.
        if (owner_ != nullptr && YGNodeGetParent(node_) == nullptr) {
            if (Widget* entryOwner = owner_->entry_owner()) {
                entryOwner->layout().mark_dirty();
            }
        }
    }

    void Layout::mark_dirty_tree() {
        mark_dirty();

        for (size_t i = 0; i < child_count(); ++i) {
            child(i)->mark_dirty_tree();
        }
    }

    void Layout::remove_from_parent() {
        IRONCAST_DEBUG_ASSERT(node_ != nullptr, "remove_from_parent called on moved-from Layout");

        if (auto parent = YGNodeGetParent(node_); parent != nullptr) {
            YGNodeRemoveChild(parent, node_);
        }
    }

    void Layout::add_child(Layout &child, size_t index) {
        IRONCAST_DEBUG_ASSERT(node_ != nullptr, "add_child called on moved-from Layout");
        IRONCAST_DEBUG_ASSERT(child.node_ != nullptr, "add_child called with moved-from child");
        IRONCAST_DEBUG_ASSERT(&child != this, "add_child: cannot add a Layout as its own child");
        IRONCAST_DEBUG_ASSERT(!YGNodeHasMeasureFunc(node_), "add_child: measured nodes must be leaves");

        size_t count = YGNodeGetChildCount(node_);
        size_t idx = index == std::string::npos ? count : index;
        IRONCAST_DEBUG_ASSERT(idx <= count, "add_child index out of range", idx, count);

        YGNodeInsertChild(node_, child.node(), idx);
    }

    void Layout::calculate_layout(float width, float height) {
        IRONCAST_DEBUG_ASSERT(node_ != nullptr, "calculate_layout called on moved-from Layout");
        IRONCAST_DEBUG_ASSERT(width >= 0.0f, "calculate_layout: negative width", width);
        IRONCAST_DEBUG_ASSERT(height >= 0.0f, "calculate_layout: negative height", height);

        YGNodeCalculateLayout(node_, width, height, YGDirectionLTR);
        calculate_position();
    }

    void Layout::set_origin(bool origin) {
        origin_ = origin;
    }

    bool Layout::origin() const {
        return origin_;
    }

    void Layout::calculate_position() {
        IRONCAST_DEBUG_ASSERT(node_ != nullptr, "calculate_position called on moved-from Layout");

        auto parentLayout = parent();

        position_.x = YGNodeLayoutGetLeft(node_);
        position_.y = YGNodeLayoutGetTop(node_);
        size_.x = YGNodeLayoutGetWidth(node_);
        size_.y = YGNodeLayoutGetHeight(node_);

        if (parentLayout != nullptr && !parentLayout->origin()) {
            position_.x += parentLayout->position().x;
            position_.y += parentLayout->position().y;
        }

        for (size_t i = 0; i < child_count(); ++i) {
            child(i)->calculate_position();
        }
    }

    float Layout::x() const {
        return position_.x;
    }

    float Layout::y() const {
        return position_.y;
    }

    const ImVec2& Layout::position() const {
        return position_;
    }

    float Layout::width() const {
        return size_.x;
    }

    float Layout::height() const {
        return size_.y;
    }

    const ImVec2& Layout::size() const {
        return size_;
    }

    Layout* Layout::parent() {
        IRONCAST_DEBUG_ASSERT(node_ != nullptr, "Layout::parent called on moved-from Layout");

        auto parent = YGNodeGetParent(node_);

        if (parent != nullptr) {
            return static_cast<Layout*>(YGNodeGetContext(parent));
        }

        return nullptr;
    }

    size_t Layout::child_count() {
        IRONCAST_DEBUG_ASSERT(node_ != nullptr, "Layout::child_count called on moved-from Layout");
        return YGNodeGetChildCount(node_);
    }

    Layout* Layout::child(size_t index) {
        IRONCAST_DEBUG_ASSERT(node_ != nullptr, "Layout::child called on moved-from Layout");
        IRONCAST_DEBUG_ASSERT(index < child_count(), "Layout::child index out of range", index, child_count());

        auto layout = static_cast<Layout*>(YGNodeGetContext(YGNodeGetChild(node_, index)));
        IRONCAST_DEBUG_ASSERT(layout != nullptr, "Yoga child node has no Layout context", index);
        return layout;
    }

    bool Layout::visible() {
        IRONCAST_DEBUG_ASSERT(node_ != nullptr, "Layout::visible called on moved-from Layout");
        return YGNodeStyleGetDisplay(node_) != YGDisplayNone;
    }

    Layout& Layout::margin(LayoutValue value, YGEdge edge) {
        switch (value.unit) {
            case YGUnitPoint:   YGNodeStyleSetMargin(node_, edge, value.value); break;
            case YGUnitPercent: YGNodeStyleSetMarginPercent(node_, edge, value.value); break;
            case YGUnitAuto:    YGNodeStyleSetMarginAuto(node_, edge); break;
            default: break;
        }
        return *this;
    }

    Layout& Layout::margin(float points, YGEdge edge) {
        return margin(ironcast::points(points), edge);
    }

    Layout& Layout::padding(LayoutValue value, YGEdge edge) {
        switch (value.unit) {
            case YGUnitPoint:   YGNodeStyleSetPadding(node_, edge, value.value); break;
            case YGUnitPercent: YGNodeStyleSetPaddingPercent(node_, edge, value.value); break;
            default: break; // padding has no "auto" in Yoga
        }
        return *this;
    }

    Layout& Layout::padding(float points, YGEdge edge) {
        return padding(ironcast::points(points), edge);
    }

    Layout& Layout::position(LayoutValue value, YGEdge edge) {
        switch (value.unit) {
            case YGUnitPoint:   YGNodeStyleSetPosition(node_, edge, value.value); break;
            case YGUnitPercent: YGNodeStyleSetPositionPercent(node_, edge, value.value); break;
            default: break; // position has no "auto" in Yoga
        }
        return *this;
    }

    Layout& Layout::position(float points, YGEdge edge) {
        return position(ironcast::points(points), edge);
    }

    Layout& Layout::width(LayoutValue value) {
        switch (value.unit) {
            case YGUnitPoint:   YGNodeStyleSetWidth(node_, value.value); break;
            case YGUnitPercent: YGNodeStyleSetWidthPercent(node_, value.value); break;
            case YGUnitAuto:    YGNodeStyleSetWidthAuto(node_); break;
            default: break;
        }
        return *this;
    }

    Layout& Layout::width(float points) {
        return width(ironcast::points(points));
    }

    Layout& Layout::height(LayoutValue value) {
        switch (value.unit) {
            case YGUnitPoint:   YGNodeStyleSetHeight(node_, value.value); break;
            case YGUnitPercent: YGNodeStyleSetHeightPercent(node_, value.value); break;
            case YGUnitAuto:    YGNodeStyleSetHeightAuto(node_); break;
            default: break;
        }
        return *this;
    }

    Layout& Layout::height(float points) {
        return height(ironcast::points(points));
    }

    Layout& Layout::gap(LayoutValue value, YGGutter gutter) {
        switch (value.unit) {
            case YGUnitPoint:   YGNodeStyleSetGap(node_, gutter, value.value); break;
            case YGUnitPercent: YGNodeStyleSetGapPercent(node_, gutter, value.value); break;
            default: break; // gap has no "auto"
        }
        return *this;
    }

    Layout& Layout::gap(float points, YGGutter gutter) {
        return gap(ironcast::points(points), gutter);
    }

    Layout& Layout::flex_direction(YGFlexDirection direction) {
        YGNodeStyleSetFlexDirection(node_, direction);
        return *this;
    }

    Layout& Layout::position_type(YGPositionType type) {
        YGNodeStyleSetPositionType(node_, type);
        return *this;
    }

    Layout& Layout::flex_grow(float value) {
        YGNodeStyleSetFlexGrow(node_, value);
        return *this;
    }

    Layout& Layout::flex_shrink(float value) {
        YGNodeStyleSetFlexShrink(node_, value);
        return *this;
    }

    Layout& Layout::flex_basis(LayoutValue value) {
        switch (value.unit) {
        case YGUnitPoint:   YGNodeStyleSetFlexBasis(node_, value.value); break;
        case YGUnitPercent: YGNodeStyleSetFlexBasisPercent(node_, value.value); break;
        case YGUnitAuto:    YGNodeStyleSetFlexBasisAuto(node_); break;
        default: break;
        }
        return *this;
    }

    Layout& Layout::flex_basis(float points) {
        return flex_basis(ironcast::points(points));
    }

    Layout& Layout::flex(float value) {
        YGNodeStyleSetFlex(node_, value);
        return *this;
    }

    Layout& Layout::flex_wrap(YGWrap wrap) {
        YGNodeStyleSetFlexWrap(node_, wrap);
        return *this;
    }

    Layout& Layout::align_items(YGAlign align) {
        YGNodeStyleSetAlignItems(node_, align);
        return *this;
    }

    Layout& Layout::align_self(YGAlign align) {
        YGNodeStyleSetAlignSelf(node_, align);
        return *this;
    }

    Layout& Layout::align_content(YGAlign align) {
        YGNodeStyleSetAlignContent(node_, align);
        return *this;
    }

    Layout& Layout::justify_content(YGJustify justify) {
        YGNodeStyleSetJustifyContent(node_, justify);
        return *this;
    }

    Layout& Layout::min_width(LayoutValue value) {
        switch (value.unit) {
        case YGUnitPoint:   YGNodeStyleSetMinWidth(node_, value.value); break;
        case YGUnitPercent: YGNodeStyleSetMinWidthPercent(node_, value.value); break;
        default: break;
        }
        return *this;
    }

    Layout& Layout::min_width(float points) {
        return min_width(ironcast::points(points));
    }

    Layout& Layout::min_height(LayoutValue value) {
        switch (value.unit) {
        case YGUnitPoint:   YGNodeStyleSetMinHeight(node_, value.value); break;
        case YGUnitPercent: YGNodeStyleSetMinHeightPercent(node_, value.value); break;
        default: break;
        }
        return *this;
    }

    Layout& Layout::min_height(float points) {
        return min_height(ironcast::points(points));
    }

    Layout& Layout::max_width(LayoutValue value) {
        switch (value.unit) {
        case YGUnitPoint:   YGNodeStyleSetMaxWidth(node_, value.value); break;
        case YGUnitPercent: YGNodeStyleSetMaxWidthPercent(node_, value.value); break;
        default: break;
        }
        return *this;
    }

    Layout& Layout::max_width(float points) {
        return max_width(ironcast::points(points));
    }

    Layout& Layout::max_height(LayoutValue value) {
        switch (value.unit) {
        case YGUnitPoint:   YGNodeStyleSetMaxHeight(node_, value.value); break;
        case YGUnitPercent: YGNodeStyleSetMaxHeightPercent(node_, value.value); break;
        default: break;
        }
        return *this;
    }

    Layout& Layout::max_height(float points) {
        return max_height(ironcast::points(points));
    }

    Layout& Layout::border(float width, YGEdge edge) {
        YGNodeStyleSetBorder(node_, edge, width);
        return *this;
    }

    Layout& Layout::display(YGDisplay display) {
        YGNodeStyleSetDisplay(node_, display);
        return *this;
    }

    Layout& Layout::overflow(YGOverflow overflow) {
        YGNodeStyleSetOverflow(node_, overflow);
        return *this;
    }

    Layout& Layout::aspect_ratio(float ratio) {
        YGNodeStyleSetAspectRatio(node_, ratio);
        return *this;
    }

    Layout& Layout::direction(YGDirection direction) {
        YGNodeStyleSetDirection(node_, direction);
        return *this;
    }

    Layout& Layout::fill_parent() {
        return width(ironcast::percent(100)).height(ironcast::percent(100));
    }

    Layout& Layout::row(float gap) {
        return flex_direction(YGFlexDirectionRow).gap(gap);
    }

    Layout& Layout::column(float gap) {
        return flex_direction(YGFlexDirectionColumn).gap(gap);
    }

    Layout& Layout::center() {
        return align_items(YGAlignCenter).justify_content(YGJustifyCenter);
    }

    float Layout::gap(YGGutter gutter) const {
        return YGNodeStyleGetGap(node_, gutter);
    }

    YGFlexDirection Layout::flex_direction() const {
        return YGNodeStyleGetFlexDirection(node_);
    }

    float Layout::flex_grow() const {
        return YGNodeStyleGetFlexGrow(node_);
    }

    float Layout::flex_shrink() const {
        return YGNodeStyleGetFlexShrink(node_);
    }

    LayoutValue Layout::flex_basis() const {
        return YGNodeStyleGetFlexBasis(node_);
    }

    float Layout::flex() const {
        return YGNodeStyleGetFlex(node_);
    }

    YGWrap Layout::flex_wrap() const {
        return YGNodeStyleGetFlexWrap(node_);
    }

    YGAlign Layout::align_items() const {
        return YGNodeStyleGetAlignItems(node_);
    }

    YGAlign Layout::align_self() const {
        return YGNodeStyleGetAlignSelf(node_);
    }

    YGAlign Layout::align_content() const {
        return YGNodeStyleGetAlignContent(node_);
    }

    YGJustify Layout::justify_content() const {
        return YGNodeStyleGetJustifyContent(node_);
    }

    LayoutValue Layout::min_width() const {
        return YGNodeStyleGetMinWidth(node_);
    }

    LayoutValue Layout::min_height() const {
        return YGNodeStyleGetMinHeight(node_);
    }

    LayoutValue Layout::max_width() const {
        return YGNodeStyleGetMaxWidth(node_);
    }

    LayoutValue Layout::max_height() const {
        return YGNodeStyleGetMaxHeight(node_);
    }

    YGDisplay Layout::display() const {
        return YGNodeStyleGetDisplay(node_);
    }

    YGOverflow Layout::overflow() const {
        return YGNodeStyleGetOverflow(node_);
    }

    float Layout::aspect_ratio() const {
        return YGNodeStyleGetAspectRatio(node_);
    }

    YGDirection Layout::direction() const {
        return YGNodeStyleGetDirection(node_);
    }

    YGPositionType Layout::position_type() const {
        return YGNodeStyleGetPositionType(node_);
    }

    float Layout::layout_margin(YGEdge edge) const {
        return YGNodeLayoutGetMargin(node_, edge);
    }

    LayoutValue Layout::style_margin(YGEdge edge) const {
        return YGNodeStyleGetMargin(node_, edge);
    }

    LayoutValue Layout::style_padding(YGEdge edge) const {
        return YGNodeStyleGetPadding(node_, edge);
    }

    float Layout::style_border(YGEdge edge) const {
        return YGNodeStyleGetBorder(node_, edge);
    }

    float Layout::layout_padding(YGEdge edge) const {
        return YGNodeLayoutGetPadding(node_, edge);
    }

    float Layout::layout_border(YGEdge edge) const {
        return YGNodeLayoutGetBorder(node_, edge);
    }
}
