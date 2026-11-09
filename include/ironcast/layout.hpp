
#pragma once

#include <yoga/Yoga.h>
#include <imgui.h>
#include <string>

namespace ironcast {
    class Widget;

    // A length with its unit: points(), percent() of the parent, or auto_(). Floats passed to the
    // setters are points.
    using LayoutValue = YGValue;

    constexpr LayoutValue points(float v){
        return LayoutValue{v, YGUnitPoint};
    }

    constexpr LayoutValue percent(float v) {
        return LayoutValue{v, YGUnitPercent};
    }
    constexpr LayoutValue auto_() {
        return LayoutValue{0.0f, YGUnitAuto};
    }

    // Resolves a measure function's intrinsic size against Yoga's constraint for one axis.
    float fit_measure(float intrinsic, float available, YGMeasureMode mode);

    namespace literals {
        constexpr LayoutValue operator""_pts(long double v) {
            return points(static_cast<float>(v));
        }

        constexpr LayoutValue operator""_pts(unsigned long long v) {
            return points(static_cast<float>(v));
        }

        constexpr LayoutValue operator""_pcts(long double v) {
            return percent(static_cast<float>(v));
        }

        constexpr LayoutValue operator""_pcts(unsigned long long v) {
            return percent(static_cast<float>(v));
        }
    }

    // A widget's Yoga node: flexbox style in, computed box out. Every Widget owns one (Widget::layout())
    // and forwards the setters, so most code uses those. The window's root layout is calculated once per
    // frame before drawing; position() is then relative to the nearest origin ancestor, size() is final.
    // Movable, not copyable: the Yoga node moves with it.
    class Layout {
        YGNodeRef node_ = nullptr;
        ImVec2 position_ = {0.0f, 0.0f};
        ImVec2 size_ = {0.0f, 0.0f};
        Widget* owner_ = nullptr;
        bool origin_ = false;

    public:
        Layout();
        ~Layout();

        Layout(Layout&& other) noexcept;
        Layout& operator=(Layout&& other) noexcept;

        Layout(const Layout& other) = delete;
        Layout& operator=(const Layout& other) = delete;

        // The Yoga node, for Yoga calls ironcast doesn't wrap.
        YGNodeRef node();

        // The widget this layout belongs to; set by Widget.
        void set_owner(Widget* owner);
        [[nodiscard]] Widget* owner() const;

        // Makes Yoga size this node by calling owner()->measure(). Leaf nodes only.
        void enable_measure();
        // Invalidates Yoga's cached measurement. No-op on nodes without a measure function, except that an
        // entry drawn outside Yoga also marks its entry owner (Widget::set_entry_owner()).
        void mark_dirty();
        // mark_dirty() on this node and every node below it.
        void mark_dirty_tree();
        // Inserts child at index, at the end by default. Not allowed on measured nodes.
        void add_child(Layout &child, size_t index = std::string::npos);
        void remove_from_parent();
        // Lays out the tree below this node in a width x height box, then calculate_position().
        void calculate_layout(float width, float height);
        // Reads the computed box of this node and the nodes below it into position() and size().
        void calculate_position();
        // An origin node starts a new coordinate space: its own position stays in the parent's
        // space, its descendants' positions are relative to it (e.g. inside an ImGui child window).
        void set_origin(bool origin);
        [[nodiscard]] bool origin() const;

        // Computed box, as of the last calculate_layout(). Position is relative to the nearest origin
        // ancestor (the window when there is none).
        [[nodiscard]] float x() const;
        [[nodiscard]] float y() const;
        [[nodiscard]] const ImVec2 &position() const;

        [[nodiscard]] float width() const;
        [[nodiscard]] float height() const;
        [[nodiscard]] const ImVec2 &size() const;

        // nullptr for the root.
        Layout *parent();
        [[nodiscard]] size_t child_count();
        // Layout of the index-th Yoga child, in insertion order.
        Layout* child(size_t index);
        // False when display(YGDisplayNone) is set: Yoga gives the node no space.
        [[nodiscard]] bool visible();

        Layout& margin(LayoutValue value, YGEdge edge = YGEdgeAll);
        Layout& margin(float points, YGEdge edge = YGEdgeAll);

        Layout& padding(LayoutValue value, YGEdge edge = YGEdgeAll);
        Layout& padding(float points, YGEdge edge = YGEdgeAll);

        Layout& position(LayoutValue value, YGEdge edge);
        Layout& position(float points, YGEdge edge);

        Layout& width(LayoutValue value);
        Layout& width(float points);
        Layout& height(LayoutValue value);
        Layout& height(float points);

        Layout& gap(LayoutValue value, YGGutter gutter = YGGutterAll);
        Layout& gap(float points, YGGutter gutter = YGGutterAll);

        Layout& flex_direction(YGFlexDirection direction);
        Layout& position_type(YGPositionType type);

        Layout& flex_grow(float value);
        Layout& flex_shrink(float value);
        Layout& flex_basis(LayoutValue value);
        Layout& flex_basis(float points);
        Layout& flex(float value);
        Layout& flex_wrap(YGWrap wrap);

        Layout& align_items(YGAlign align);
        Layout& align_self(YGAlign align);
        Layout& align_content(YGAlign align);
        Layout& justify_content(YGJustify justify);

        Layout& min_width(LayoutValue value);
        Layout& min_width(float points);
        Layout& min_height(LayoutValue value);
        Layout& min_height(float points);
        Layout& max_width(LayoutValue value);
        Layout& max_width(float points);
        Layout& max_height(LayoutValue value);
        Layout& max_height(float points);

        Layout& border(float width, YGEdge edge = YGEdgeAll);
        Layout& display(YGDisplay display);
        Layout& overflow(YGOverflow overflow);
        Layout& aspect_ratio(float ratio);
        Layout& direction(YGDirection direction);

        // Shortcuts. fill_parent(): 100% width and height. row() / column(): flex direction and gap.
        // center(): children centred on both axes (align_items and justify_content).
        Layout& fill_parent();
        Layout& row(float gap = 0.0f);
        Layout& column(float gap = 0.0f);
        Layout& center();

        // Style values as set. Unset values come back as YGUnitUndefined / NaN, as Yoga reports them.
        [[nodiscard]] float gap(YGGutter gutter = YGGutterAll) const;
        [[nodiscard]] YGFlexDirection flex_direction() const;
        [[nodiscard]] float flex_grow() const;
        [[nodiscard]] float flex_shrink() const;
        [[nodiscard]] LayoutValue flex_basis() const;
        [[nodiscard]] float flex() const;
        [[nodiscard]] YGWrap flex_wrap() const;
        [[nodiscard]] YGAlign align_items() const;
        [[nodiscard]] YGAlign align_self() const;
        [[nodiscard]] YGAlign align_content() const;
        [[nodiscard]] YGJustify justify_content() const;
        [[nodiscard]] LayoutValue min_width() const;
        [[nodiscard]] LayoutValue min_height() const;
        [[nodiscard]] LayoutValue max_width() const;
        [[nodiscard]] LayoutValue max_height() const;
        [[nodiscard]] YGDisplay display() const;
        [[nodiscard]] YGOverflow overflow() const;
        [[nodiscard]] float aspect_ratio() const;
        [[nodiscard]] YGDirection direction() const;
        [[nodiscard]] YGPositionType position_type() const;
        // Only what was set on that edge itself: an edge set through YGEdgeAll (or Horizontal,
        // Vertical, ...) comes back unset.
        [[nodiscard]] LayoutValue style_margin(YGEdge edge) const;
        [[nodiscard]] LayoutValue style_padding(YGEdge edge) const;
        [[nodiscard]] float style_border(YGEdge edge) const;

        // Computed by the last calculate_layout().
        [[nodiscard]] float layout_margin(YGEdge edge) const;
        [[nodiscard]] float layout_padding(YGEdge edge) const;
        [[nodiscard]] float layout_border(YGEdge edge) const;
    };
}
