
#pragma once

#include <cstdint>
#include <string>
#include <concepts>
#include <unordered_map>

#include "ironcast/layout.hpp"
#include "ironcast/style.hpp"

namespace ironcast {
    class Window;
    class Ironcast;

    // Base of every ironcast widget: a node in the window's Yoga tree with a title, an id, a Style
    // and per-widget state (enabled, tooltip).
    //
    // - Layout: the flexbox setters below forward to layout() and return *this, so they chain.
    //   Children are added by passing the parent to the constructor or with set_parent(); they
    //   render in insertion order.
    // - Drawing: render() places the widget at its Yoga box and calls draw(), which applies the
    //   style, enabled state and tooltip around show(). Containers use the default show(), which
    //   renders the children; leaves override show() to draw themselves with ImGui and measure()
    //   to give Yoga their natural size.
    // - The title is the ImGui label; text after "##" is hidden, as in ImGui.
    // - Widgets can be moved but not copied; the id stays the same across moves, so post() and
    //   find() keep working. UI thread only.
    class Widget {
        uint64_t id_ = 0;
        std::string title_;
        std::string titleID_;
        std::string tooltip_;
        bool enabled_ = true;

        Window* window_ = nullptr;
        // Top-left of the Yoga box in screen space, recorded by render().
        ImVec2 screenPos_{0.0f, 0.0f};
        // The widget that measures this one when it is drawn outside the Yoga tree (a menu's bar, a cell's
        // table); 0 = none. An id, so a destroyed owner is just skipped.
        uint64_t entryOwner_ = 0;

        Layout layout_;
        Style style_{this};

        // Live widgets by id, for post() and find().
        static inline std::unordered_map<uint64_t, Widget*> registry_;

    protected:
        // Width of the visible label (text after "##" hidden), 0 when there is none. ImGui context required.
        [[nodiscard]] float label_width() const;
        // Space ImGui puts after a field for its label: ItemInnerSpacing.x + label, or 0 when there is none.
        [[nodiscard]] float label_extent() const;
        // Intrinsic field width for frame widgets that have no natural width (Slider, Drag, ColorEdit, ...).
        [[nodiscard]] static float default_field_width();

        // Measure for a one-line frame widget: default field width + labelExtent, frame height.
        [[nodiscard]] static ImVec2 measure_field(
            float labelExtent,
            float width,
            YGMeasureMode widthMode,
            float height,
            YGMeasureMode heightMode
        );
        // SetNextItemWidth for the field part of this widget's box, leaving labelExtent for the label.
        void set_next_field_width(float labelExtent) const;

        // One-line frame widgets take no height from ImGui: they are drawn at GetFrameHeight().
        // When this widget's box is taller or shorter, push FramePadding.y so the frame fills it.
        // Returns whether a style var was pushed; pass the result to pop_frame_height().
        [[nodiscard]] bool push_frame_height() const;
        static void pop_frame_height(bool pushed);
        // Frame height push_frame_height() will draw at, for measuring frame-height squares
        // (combo arrow, color swatch) when Yoga passes an exact height.
        [[nodiscard]] static float frame_height(float height, YGMeasureMode heightMode);

    public:
        Widget(const std::string &title = "", Widget *parent = nullptr);

        Widget(const Widget& other) = delete;
        Widget &operator=(const Widget& other) = delete;

        Widget(Widget&& other) noexcept;
        Widget& operator=(Widget&& other) noexcept;

        virtual ~Widget();
        // Default: calls init() on the children in Yoga order, destroy() in reverse order.
        // Overrides call the base version to keep reaching the children.
        virtual void init();
        virtual void destroy();
        // Natural size for Yoga, called only for widgets that enabled measuring (leaves).
        virtual ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode);
        [[nodiscard]] uint64_t id() const;
        // The live widget with this id, or nullptr once it is destroyed.
        [[nodiscard]] static Widget* find(uint64_t id);

        // Runs fn on the next dispatch with this widget, as its own type, if it still exists then;
        // the action is dropped if the widget was destroyed meanwhile. Actions run in call order.
        // A moved widget keeps its id, so fn gets the moved-to object. Use self in fn, never a
        // captured this: without this captured, any member access through it doesn't compile.
        // delay works as in Ironcast::add_action(): posted from a running action, delay 0 runs in the
        // same dispatch, so re-posting itself every frame needs delay 1.
        template <class Self, std::invocable<Self&> F>
        void post(this Self& self, F fn, uint64_t delay = 0) {
            self.app().add_action([id = self.id(), fn = std::move(fn)]() mutable {
                if (Widget* widget = find(id)) {
                    fn(static_cast<Self&>(*widget));
                }
            }, delay);
        }
        [[nodiscard]] const std::string& title() const;
        [[nodiscard]] const std::string& title_id() const;
        Widget& set_title(const std::string& title);
        // Text shown while the widget's box is hovered. Empty means no tooltip.
        [[nodiscard]] const std::string& tooltip() const;
        Widget& set_tooltip(const std::string& tooltip);
        // A disabled widget is greyed out and ignores input; so is everything drawn inside it.
        [[nodiscard]] bool enabled() const;
        Widget& set_enabled(bool enabled);
        // Positions the widget at its Yoga box, then draw()s it.
        bool render();
        // show() with the per-widget state applied (disabled, tooltip). Owners that draw
        // widgets outside the Yoga tree (menus, table cells) call this instead of render().
        bool draw();
        // Default: renders the children in Yoga order. Leaves override it to draw themselves.
        virtual bool show();
        Window* window();
        void set_window(Window* window);
        Widget* parent();
        void set_parent(Widget* parent);
        // For widgets drawn outside the Yoga tree: owner measures them, so it is marked dirty whenever they
        // are (title, style). Set by the owner when it adds the entry. Unrelated to parent().
        void set_entry_owner(Widget& owner);
        // The live entry owner, or nullptr.
        [[nodiscard]] Widget* entry_owner();
        // Whether this widget's tree is its window's root tree, so the window's init() has reached (or is
        // reaching) it. A widget added to such a tree later needs its own init().
        [[nodiscard]] bool in_window_tree();
        Ironcast& app();
        Layout& layout();
        // Pushed around show(), so it applies to the children too; measure sees it as well.
        Style& style();

        Widget& margin(LayoutValue value, YGEdge edge = YGEdgeAll);
        Widget& margin(float points, YGEdge edge = YGEdgeAll);

        Widget& padding(LayoutValue value, YGEdge edge = YGEdgeAll);
        Widget& padding(float points, YGEdge edge = YGEdgeAll);

        Widget& position(LayoutValue value, YGEdge edge);
        Widget& position(float points, YGEdge edge);

        Widget& width(LayoutValue value);
        Widget& width(float points);
        Widget& height(LayoutValue value);
        Widget& height(float points);

        Widget& gap(LayoutValue value, YGGutter gutter = YGGutterAll);
        Widget& gap(float points, YGGutter gutter = YGGutterAll);

        Widget& flex_direction(YGFlexDirection direction);
        Widget& flex_grow(float value);
        Widget& flex_shrink(float value);
        Widget& flex_basis(LayoutValue value);
        Widget& flex_basis(float points);
        Widget& flex(float value);
        Widget& flex_wrap(YGWrap wrap);

        Widget& align_items(YGAlign align);
        Widget& align_self(YGAlign align);
        Widget& align_content(YGAlign align);
        Widget& justify_content(YGJustify justify);

        Widget& min_width(LayoutValue value);
        Widget& min_width(float points);
        Widget& min_height(LayoutValue value);
        Widget& min_height(float points);
        Widget& max_width(LayoutValue value);
        Widget& max_width(float points);
        Widget& max_height(LayoutValue value);
        Widget& max_height(float points);

        Widget& border(float width, YGEdge edge = YGEdgeAll);
        Widget& display(YGDisplay display);
        Widget& overflow(YGOverflow overflow);
        Widget& aspect_ratio(float ratio);
        Widget& direction(YGDirection direction);
        Widget& position_type(YGPositionType type);

        Widget& fill_parent();
        Widget& row(float gap = 0.0f);
        Widget& column(float gap = 0.0f);
        Widget& center();

        // Style values as set.
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

        [[nodiscard]] float x() const;
        [[nodiscard]] float y() const;
        [[nodiscard]] float width() const;
        [[nodiscard]] float height() const;
        [[nodiscard]] const ImVec2& position() const;
        [[nodiscard]] const ImVec2& size() const;
        // Top-left of the box in screen space as of the last render(). Not updated for widgets
        // drawn through draw() only (menu entries, table cells).
        [[nodiscard]] const ImVec2& screen_position() const;
        [[nodiscard]] float layout_margin(YGEdge edge) const;
        [[nodiscard]] float layout_padding(YGEdge edge) const;
        [[nodiscard]] float layout_border(YGEdge edge) const;

        // Style shortcuts, see Style.
        Widget& color(ImGuiCol idx, std::optional<ImVec4> color);
        Widget& set_font(ImFont* font);
        Widget& set_font_size(float value);
        Widget& set_alpha(std::optional<float> value);
        Widget& set_disabled_alpha(std::optional<float> value);
        Widget& set_window_rounding(std::optional<float> value);
        Widget& set_window_border_size(std::optional<float> value);
        Widget& set_child_rounding(std::optional<float> value);
        Widget& set_child_border_size(std::optional<float> value);
        Widget& set_popup_rounding(std::optional<float> value);
        Widget& set_popup_border_size(std::optional<float> value);
        Widget& set_frame_rounding(std::optional<float> value);
        Widget& set_frame_border_size(std::optional<float> value);
        Widget& set_indent_spacing(std::optional<float> value);
        Widget& set_scrollbar_size(std::optional<float> value);
        Widget& set_scrollbar_rounding(std::optional<float> value);
        Widget& set_scrollbar_padding(std::optional<float> value);
        Widget& set_grab_min_size(std::optional<float> value);
        Widget& set_grab_rounding(std::optional<float> value);
        Widget& set_image_rounding(std::optional<float> value);
        Widget& set_image_border_size(std::optional<float> value);
        Widget& set_tab_rounding(std::optional<float> value);
        Widget& set_tab_border_size(std::optional<float> value);
        Widget& set_tab_min_width_base(std::optional<float> value);
        Widget& set_tab_min_width_shrink(std::optional<float> value);
        Widget& set_tab_bar_border_size(std::optional<float> value);
        Widget& set_tab_bar_overline_size(std::optional<float> value);
        Widget& set_table_angled_headers_angle(std::optional<float> value);
        Widget& set_tree_lines_size(std::optional<float> value);
        Widget& set_tree_lines_rounding(std::optional<float> value);
        Widget& set_menu_item_rounding(std::optional<float> value);
        Widget& set_selectable_rounding(std::optional<float> value);
        Widget& set_drag_drop_target_rounding(std::optional<float> value);
        Widget& set_separator_size(std::optional<float> value);
        Widget& set_separator_text_border_size(std::optional<float> value);
        Widget& set_docking_separator_size(std::optional<float> value);
        Widget& set_window_padding(std::optional<ImVec2> value);
        Widget& set_window_min_size(std::optional<ImVec2> value);
        Widget& set_window_title_align(std::optional<ImVec2> value);
        Widget& set_frame_padding(std::optional<ImVec2> value);
        Widget& set_item_spacing(std::optional<ImVec2> value);
        Widget& set_item_inner_spacing(std::optional<ImVec2> value);
        Widget& set_cell_padding(std::optional<ImVec2> value);
        Widget& set_table_angled_headers_text_align(std::optional<ImVec2> value);
        Widget& set_button_text_align(std::optional<ImVec2> value);
        Widget& set_selectable_text_align(std::optional<ImVec2> value);
        Widget& set_separator_text_align(std::optional<ImVec2> value);
        Widget& set_separator_text_padding(std::optional<ImVec2> value);

        [[nodiscard]] std::optional<ImVec4> color(ImGuiCol idx) const;
        [[nodiscard]] ImFont* font() const;
        [[nodiscard]] float font_size() const;
        [[nodiscard]] std::optional<float> alpha() const;
        [[nodiscard]] std::optional<float> disabled_alpha() const;
        [[nodiscard]] std::optional<float> window_rounding() const;
        [[nodiscard]] std::optional<float> window_border_size() const;
        [[nodiscard]] std::optional<float> child_rounding() const;
        [[nodiscard]] std::optional<float> child_border_size() const;
        [[nodiscard]] std::optional<float> popup_rounding() const;
        [[nodiscard]] std::optional<float> popup_border_size() const;
        [[nodiscard]] std::optional<float> frame_rounding() const;
        [[nodiscard]] std::optional<float> frame_border_size() const;
        [[nodiscard]] std::optional<float> indent_spacing() const;
        [[nodiscard]] std::optional<float> scrollbar_size() const;
        [[nodiscard]] std::optional<float> scrollbar_rounding() const;
        [[nodiscard]] std::optional<float> scrollbar_padding() const;
        [[nodiscard]] std::optional<float> grab_min_size() const;
        [[nodiscard]] std::optional<float> grab_rounding() const;
        [[nodiscard]] std::optional<float> image_rounding() const;
        [[nodiscard]] std::optional<float> image_border_size() const;
        [[nodiscard]] std::optional<float> tab_rounding() const;
        [[nodiscard]] std::optional<float> tab_border_size() const;
        [[nodiscard]] std::optional<float> tab_min_width_base() const;
        [[nodiscard]] std::optional<float> tab_min_width_shrink() const;
        [[nodiscard]] std::optional<float> tab_bar_border_size() const;
        [[nodiscard]] std::optional<float> tab_bar_overline_size() const;
        [[nodiscard]] std::optional<float> table_angled_headers_angle() const;
        [[nodiscard]] std::optional<float> tree_lines_size() const;
        [[nodiscard]] std::optional<float> tree_lines_rounding() const;
        [[nodiscard]] std::optional<float> menu_item_rounding() const;
        [[nodiscard]] std::optional<float> selectable_rounding() const;
        [[nodiscard]] std::optional<float> drag_drop_target_rounding() const;
        [[nodiscard]] std::optional<float> separator_size() const;
        [[nodiscard]] std::optional<float> separator_text_border_size() const;
        [[nodiscard]] std::optional<float> docking_separator_size() const;
        [[nodiscard]] std::optional<ImVec2> window_padding() const;
        [[nodiscard]] std::optional<ImVec2> window_min_size() const;
        [[nodiscard]] std::optional<ImVec2> window_title_align() const;
        [[nodiscard]] std::optional<ImVec2> frame_padding() const;
        [[nodiscard]] std::optional<ImVec2> item_spacing() const;
        [[nodiscard]] std::optional<ImVec2> item_inner_spacing() const;
        [[nodiscard]] std::optional<ImVec2> cell_padding() const;
        [[nodiscard]] std::optional<ImVec2> table_angled_headers_text_align() const;
        [[nodiscard]] std::optional<ImVec2> button_text_align() const;
        [[nodiscard]] std::optional<ImVec2> selectable_text_align() const;
        [[nodiscard]] std::optional<ImVec2> separator_text_align() const;
        [[nodiscard]] std::optional<ImVec2> separator_text_padding() const;
    };


    template <class W>
    concept WidgetConcept = std::derived_from<W, Widget>;
}
