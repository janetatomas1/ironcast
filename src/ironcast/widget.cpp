
#include <algorithm>
#include <cmath>
#include <format>

#include "ironcast/assert.hpp"

#include "ironcast/widget.hpp"
#include "ironcast/ironcast.hpp"
#include "ironcast/window.hpp"

namespace ironcast {
    Widget::Widget(const std::string& title, Widget* parent):
    id_(Ironcast::random()),
    title_(title),
    titleID_(std::format("{}##{}", title, id_)) {
        const bool inserted = registry_.emplace(id_, this).second;
        IRONCAST_DEBUG_ASSERT(inserted, "Widget id collision", id_);

        layout_.set_owner(this);
        if (parent != nullptr) {
            parent->layout().add_child(layout());
        }
    }

    Widget::Widget(Widget&& other) noexcept :
        id_(other.id_),
        title_(std::move(other.title_)),
        titleID_(std::move(other.titleID_)),
        tooltip_(std::move(other.tooltip_)),
        enabled_(other.enabled_),
        window_(other.window_),
        screenPos_(other.screenPos_),
        entryOwner_(other.entryOwner_),
        layout_(std::move(other.layout_)),
        style_(std::move(other.style_)) {
        layout_.set_owner(this);
        style_.set_widget(this);
        // The id moves with the widget; the moved-from one no longer owns an entry.
        registry_[id_] = this;
        other.id_ = 0;
    }

    Widget::~Widget() {
        if (id_ != 0) {
            registry_.erase(id_);
        }
    }

    Widget& Widget::operator=(Widget&& other) noexcept {
        if (this != &other) {
            // This widget's own actions are dropped; queued ones for other now reach it.
            if (id_ != 0) {
                registry_.erase(id_);
            }
            id_ = other.id_;
            registry_[id_] = this;
            other.id_ = 0;
            title_ = std::move(other.title_);
            titleID_ = std::move(other.titleID_);
            tooltip_ = std::move(other.tooltip_);
            enabled_ = other.enabled_;
            window_ = other.window_;
            screenPos_ = other.screenPos_;
            entryOwner_ = other.entryOwner_;
            layout_ = std::move(other.layout_);
            layout_.set_owner(this);
            style_ = std::move(other.style_);
            style_.set_widget(this);
        }

        return *this;
    }

    void Widget::init() {
        for (size_t i = 0; i < layout_.child_count(); ++i) {
            auto owner = layout_.child(i)->owner();
            IRONCAST_DEBUG_ASSERT(owner != nullptr, "Widget::init: child Layout has no owner", i);
            owner->init();
        }
    }

    void Widget::destroy() {
        // Reverse of init(): later children may depend on earlier ones.
        for (size_t i = layout_.child_count(); i-- > 0;) {
            auto owner = layout_.child(i)->owner();
            IRONCAST_DEBUG_ASSERT(owner != nullptr, "Widget::destroy: child Layout has no owner", i);
            owner->destroy();
        }
    }

    ImVec2 Widget::measure(float, YGMeasureMode, float, YGMeasureMode) {
        return ImVec2{0.0f, 0.0f};
    }

    float Widget::label_width() const {
        return ImGui::CalcTextSize(titleID_.c_str(), nullptr, true).x;
    }

    float Widget::label_extent() const {
        const float label = label_width();
        return label > 0.0f ? ImGui::GetStyle().ItemInnerSpacing.x + label : 0.0f;
    }

    float Widget::default_field_width() {
        // ImGui's own fallback item width for auto-resizing windows.
        return ImGui::GetFontSize() * 16.0f;
    }

    ImVec2 Widget::measure_field(
        float labelExtent,
        float width,
        YGMeasureMode widthMode,
        float height,
        YGMeasureMode heightMode
    ) {
        return ImVec2{
            fit_measure(default_field_width() + labelExtent, width, widthMode),
            fit_measure(ImGui::GetFrameHeight(), height, heightMode)
        };
    }

    void Widget::set_next_field_width(float labelExtent) const {
        // ImGui draws the label outside the item width, so the field gets what is left of the box.
        // Must stay positive: ImGui treats a width <= 0 as relative to the window's right edge.
        if (width() > 0.0f) {
            ImGui::SetNextItemWidth(std::max(width() - labelExtent, 1.0f));
        }
    }

    bool Widget::push_frame_height() const {
        // Measured heights already equal the frame height; only explicit, stretched or
        // flexed heights need a push. Sub-pixel differences come from Yoga's rounding.
        if (height() <= 0.0f || std::abs(height() - ImGui::GetFrameHeight()) < 0.5f) {
            return false;
        }

        // A box shorter than the font can't be honoured; the frame gets no vertical padding.
        const ImVec2 padding = ImGui::GetStyle().FramePadding;
        const float paddingY = std::max((height() - ImGui::GetFontSize()) * 0.5f, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{padding.x, paddingY});
        return true;
    }

    void Widget::pop_frame_height(bool pushed) {
        if (pushed) {
            ImGui::PopStyleVar();
        }
    }

    float Widget::frame_height(float height, YGMeasureMode heightMode) {
        if (heightMode != YGMeasureModeExactly || std::abs(height - ImGui::GetFrameHeight()) < 0.5f) {
            return ImGui::GetFrameHeight();
        }

        return std::max(height, ImGui::GetFontSize());
    }

    uint64_t Widget::id() const {
        return id_;
    }

    Widget* Widget::find(uint64_t id) {
        auto it = registry_.find(id);
        return it != registry_.end() ? it->second : nullptr;
    }

    const std::string& Widget::title() const {
        return title_;
    }

    const std::string& Widget::title_id() const {
        return titleID_;
    }

    Widget& Widget::set_title(const std::string& title) {
        title_ = title;
        titleID_ = std::format("{}##{}", title_, id_);
        layout_.mark_dirty();
        return *this;
    }

    const std::string& Widget::tooltip() const {
        return tooltip_;
    }

    Widget& Widget::set_tooltip(const std::string& tooltip) {
        tooltip_ = tooltip;
        return *this;
    }

    bool Widget::enabled() const {
        return enabled_;
    }

    Widget& Widget::set_enabled(bool enabled) {
        enabled_ = enabled;
        return *this;
    }

    bool Widget::render() {
        ImGui::SetCursorPos(layout().position());
        screenPos_ = ImGui::GetCursorScreenPos();
        return draw();
    }

    bool Widget::draw() {
        style_.push();

        if (!enabled_) {
            ImGui::BeginDisabled();
        }

        bool changed = false;

        if (tooltip_.empty()) {
            changed = show();
        } else {
            // The group makes the whole widget one item, so hovering works for containers too,
            // not just for their last child.
            ImGui::BeginGroup();
            changed = show();
            ImGui::EndGroup();
        }

        // Ended before the tooltip, which would otherwise be drawn greyed out too.
        if (!enabled_) {
            ImGui::EndDisabled();
        }

        // Popped before the tooltip too, which is its own window and keeps ImGui's style.
        style_.pop();

        if (!tooltip_.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("%s", tooltip_.c_str());
        }

        return changed;
    }

    bool Widget::show() {
        bool changed = false;

        // Count re-read each step: a child's callback may change the tree (actions are
        // normally deferred, but don't rely on it for bounds).
        for (size_t i = 0; i < layout_.child_count(); ++i) {
            auto child = layout_.child(i);
            if (!child->visible()) {
                continue; // Yoga gave it no box; drawing it would land on a stale position.
            }

            auto owner = child->owner();
            IRONCAST_DEBUG_ASSERT(owner != nullptr, "Widget::show: child Layout has no owner", i);
            changed |= owner->render();
        }

        return changed;
    }

    Window* Widget::window() {
        if (window_ != nullptr) {
            return window_;
        }

        auto p = parent();
        IRONCAST_DEBUG_ASSERT(p != nullptr, "Widget::window: no window_ and no parent to fall back to", title_);
        return p->window();
    }

    void Widget::set_window(Window* window) {
        window_ = window;
    }

    Widget* Widget::parent() {
        auto parentLayout = layout_.parent();
        return parentLayout != nullptr ? parentLayout->owner() : nullptr;
    }

    void Widget::set_parent(Widget* parent) {
        IRONCAST_DEBUG_ASSERT(parent != this, "set_parent: cannot set a Widget as its own parent");

        // The Yoga tree is the only record of parenthood. Only touch it when
        // the parent actually changes, so re-asserting the same parent
        // doesn't silently reorder this child to the end of the children list.
        auto currentParentNode = YGNodeGetParent(layout_.node());
        auto newParentNode = parent != nullptr ? parent->layout().node() : nullptr;

        if (currentParentNode != newParentNode) {
            layout_.remove_from_parent();
            if (parent != nullptr) {
                parent->layout().add_child(layout_);
            }

            // The new tree may belong to a window with a different style.
            layout_.mark_dirty_tree();
        }
    }

    void Widget::set_entry_owner(Widget& owner) {
        IRONCAST_DEBUG_ASSERT(&owner != this, "set_entry_owner: a Widget cannot own itself");
        entryOwner_ = owner.id();
    }

    Widget* Widget::entry_owner() {
        return entryOwner_ != 0 ? find(entryOwner_) : nullptr;
    }

    bool Widget::in_window_tree() {
        Widget* root = this;
        while (Widget* p = root->parent()) {
            root = p;
        }
        return root->window_ != nullptr && root->window_->root_widget() == root;
    }

    Ironcast& Widget::app() {
        return Ironcast::current();
    }

    Layout& Widget::layout() {
        return layout_;
    }

    Style& Widget::style() {
        return style_;
    }

    Widget& Widget::margin(LayoutValue value, YGEdge edge) {
        layout_.margin(value, edge);
        return *this;
    }

    Widget& Widget::margin(float points, YGEdge edge) {
        layout_.margin(points, edge);
        return *this;
    }

    Widget& Widget::padding(LayoutValue value, YGEdge edge) {
        layout_.padding(value, edge);
        return *this;
    }

    Widget& Widget::padding(float points, YGEdge edge) {
        layout_.padding(points, edge);
        return *this;
    }

    Widget& Widget::position(LayoutValue value, YGEdge edge) {
        layout_.position(value, edge);
        return *this;
    }

    Widget& Widget::position(float points, YGEdge edge) {
        layout_.position(points, edge);
        return *this;
    }

    Widget& Widget::width(LayoutValue value) {
        layout_.width(value);
        return *this;
    }

    Widget& Widget::width(float points) {
        layout_.width(points);
        return *this;
    }

    Widget& Widget::height(LayoutValue value) {
        layout_.height(value);
        return *this;
    }

    Widget& Widget::height(float points) {
        layout_.height(points);
        return *this;
    }

    Widget& Widget::gap(LayoutValue value, YGGutter gutter) {
        layout_.gap(value, gutter);
        return *this;
    }

    Widget& Widget::gap(float points, YGGutter gutter) {
        layout_.gap(points, gutter);
        return *this;
    }

    Widget& Widget::flex_direction(YGFlexDirection direction) {
        layout_.flex_direction(direction);
        return *this;
    }

    Widget& Widget::flex_grow(float value) {
        layout_.flex_grow(value);
        return *this;
    }

    Widget& Widget::flex_shrink(float value) {
        layout_.flex_shrink(value);
        return *this;
    }

    Widget& Widget::flex_basis(LayoutValue value) {
        layout_.flex_basis(value);
        return *this;
    }

    Widget& Widget::flex_basis(float points) {
        layout_.flex_basis(points);
        return *this;
    }

    Widget& Widget::flex(float value) {
        layout_.flex(value);
        return *this;
    }

    Widget& Widget::flex_wrap(YGWrap wrap) {
        layout_.flex_wrap(wrap);
        return *this;
    }

    Widget& Widget::align_items(YGAlign align) {
        layout_.align_items(align);
        return *this;
    }

    Widget& Widget::align_self(YGAlign align) {
        layout_.align_self(align);
        return *this;
    }

    Widget& Widget::align_content(YGAlign align) {
        layout_.align_content(align);
        return *this;
    }

    Widget& Widget::justify_content(YGJustify justify) {
        layout_.justify_content(justify);
        return *this;
    }

    Widget& Widget::min_width(LayoutValue value) {
        layout_.min_width(value);
        return *this;
    }

    Widget& Widget::min_width(float points) {
        layout_.min_width(points);
        return *this;
    }

    Widget& Widget::min_height(LayoutValue value) {
        layout_.min_height(value);
        return *this;
    }

    Widget& Widget::min_height(float points) {
        layout_.min_height(points);
        return *this;
    }

    Widget& Widget::max_width(LayoutValue value) {
        layout_.max_width(value);
        return *this;
    }

    Widget& Widget::max_width(float points) {
        layout_.max_width(points);
        return *this;
    }

    Widget& Widget::max_height(LayoutValue value) {
        layout_.max_height(value);
        return *this;
    }

    Widget& Widget::max_height(float points) {
        layout_.max_height(points);
        return *this;
    }

    Widget& Widget::border(float width, YGEdge edge) {
        layout_.border(width, edge);
        return *this;
    }

    Widget& Widget::display(YGDisplay display) {
        layout_.display(display);
        return *this;
    }

    Widget& Widget::overflow(YGOverflow overflow) {
        layout_.overflow(overflow);
        return *this;
    }

    Widget& Widget::aspect_ratio(float ratio) {
        layout_.aspect_ratio(ratio);
        return *this;
    }

    Widget& Widget::direction(YGDirection direction) {
        layout_.direction(direction);
        return *this;
    }

    Widget& Widget::position_type(YGPositionType type) {
        layout_.position_type(type);
        return *this;
    }

    Widget& Widget::fill_parent() {
        layout_.fill_parent();
        return *this;
    }

    Widget& Widget::row(float gap) {
        layout_.row(gap);
        return *this;
    }

    Widget& Widget::column(float gap) {
        layout_.column(gap);
        return *this;
    }

    Widget& Widget::center() {
        layout_.center();
        return *this;
    }

    float Widget::gap(YGGutter gutter) const {
        return layout_.gap(gutter);
    }

    YGFlexDirection Widget::flex_direction() const {
        return layout_.flex_direction();
    }

    float Widget::flex_grow() const {
        return layout_.flex_grow();
    }

    float Widget::flex_shrink() const {
        return layout_.flex_shrink();
    }

    LayoutValue Widget::flex_basis() const {
        return layout_.flex_basis();
    }

    float Widget::flex() const {
        return layout_.flex();
    }

    YGWrap Widget::flex_wrap() const {
        return layout_.flex_wrap();
    }

    YGAlign Widget::align_items() const {
        return layout_.align_items();
    }

    YGAlign Widget::align_self() const {
        return layout_.align_self();
    }

    YGAlign Widget::align_content() const {
        return layout_.align_content();
    }

    YGJustify Widget::justify_content() const {
        return layout_.justify_content();
    }

    LayoutValue Widget::min_width() const {
        return layout_.min_width();
    }

    LayoutValue Widget::min_height() const {
        return layout_.min_height();
    }

    LayoutValue Widget::max_width() const {
        return layout_.max_width();
    }

    LayoutValue Widget::max_height() const {
        return layout_.max_height();
    }

    YGDisplay Widget::display() const {
        return layout_.display();
    }

    YGOverflow Widget::overflow() const {
        return layout_.overflow();
    }

    float Widget::aspect_ratio() const {
        return layout_.aspect_ratio();
    }

    YGDirection Widget::direction() const {
        return layout_.direction();
    }

    YGPositionType Widget::position_type() const {
        return layout_.position_type();
    }

    float Widget::x() const {
        return layout_.x();
    }

    float Widget::y() const {
        return layout_.y();
    }

    float Widget::width() const {
        return layout_.width();
    }

    float Widget::height() const {
        return layout_.height();
    }

    const ImVec2& Widget::position() const {
        return layout_.position();
    }

    const ImVec2& Widget::size() const {
        return layout_.size();
    }

    const ImVec2& Widget::screen_position() const {
        return screenPos_;
    }

    float Widget::layout_margin(YGEdge edge) const {
        return layout_.layout_margin(edge);
    }

    float Widget::layout_padding(YGEdge edge) const {
        return layout_.layout_padding(edge);
    }

    float Widget::layout_border(YGEdge edge) const {
        return layout_.layout_border(edge);
    }

    Widget& Widget::color(ImGuiCol idx, std::optional<ImVec4> color) {
        style_.color(idx, color);
        return *this;
    }

    Widget& Widget::set_font(ImFont* font) {
        style_.set_font(font);
        return *this;
    }

    Widget& Widget::set_font_size(float value) {
        style_.set_font_size(value);
        return *this;
    }

    Widget& Widget::set_alpha(std::optional<float> value) {
        style_.set_alpha(value);
        return *this;
    }

    Widget& Widget::set_disabled_alpha(std::optional<float> value) {
        style_.set_disabled_alpha(value);
        return *this;
    }

    Widget& Widget::set_window_rounding(std::optional<float> value) {
        style_.set_window_rounding(value);
        return *this;
    }

    Widget& Widget::set_window_border_size(std::optional<float> value) {
        style_.set_window_border_size(value);
        return *this;
    }

    Widget& Widget::set_child_rounding(std::optional<float> value) {
        style_.set_child_rounding(value);
        return *this;
    }

    Widget& Widget::set_child_border_size(std::optional<float> value) {
        style_.set_child_border_size(value);
        return *this;
    }

    Widget& Widget::set_popup_rounding(std::optional<float> value) {
        style_.set_popup_rounding(value);
        return *this;
    }

    Widget& Widget::set_popup_border_size(std::optional<float> value) {
        style_.set_popup_border_size(value);
        return *this;
    }

    Widget& Widget::set_frame_rounding(std::optional<float> value) {
        style_.set_frame_rounding(value);
        return *this;
    }

    Widget& Widget::set_frame_border_size(std::optional<float> value) {
        style_.set_frame_border_size(value);
        return *this;
    }

    Widget& Widget::set_indent_spacing(std::optional<float> value) {
        style_.set_indent_spacing(value);
        return *this;
    }

    Widget& Widget::set_scrollbar_size(std::optional<float> value) {
        style_.set_scrollbar_size(value);
        return *this;
    }

    Widget& Widget::set_scrollbar_rounding(std::optional<float> value) {
        style_.set_scrollbar_rounding(value);
        return *this;
    }

    Widget& Widget::set_scrollbar_padding(std::optional<float> value) {
        style_.set_scrollbar_padding(value);
        return *this;
    }

    Widget& Widget::set_grab_min_size(std::optional<float> value) {
        style_.set_grab_min_size(value);
        return *this;
    }

    Widget& Widget::set_grab_rounding(std::optional<float> value) {
        style_.set_grab_rounding(value);
        return *this;
    }

    Widget& Widget::set_image_rounding(std::optional<float> value) {
        style_.set_image_rounding(value);
        return *this;
    }

    Widget& Widget::set_image_border_size(std::optional<float> value) {
        style_.set_image_border_size(value);
        return *this;
    }

    Widget& Widget::set_tab_rounding(std::optional<float> value) {
        style_.set_tab_rounding(value);
        return *this;
    }

    Widget& Widget::set_tab_border_size(std::optional<float> value) {
        style_.set_tab_border_size(value);
        return *this;
    }

    Widget& Widget::set_tab_min_width_base(std::optional<float> value) {
        style_.set_tab_min_width_base(value);
        return *this;
    }

    Widget& Widget::set_tab_min_width_shrink(std::optional<float> value) {
        style_.set_tab_min_width_shrink(value);
        return *this;
    }

    Widget& Widget::set_tab_bar_border_size(std::optional<float> value) {
        style_.set_tab_bar_border_size(value);
        return *this;
    }

    Widget& Widget::set_tab_bar_overline_size(std::optional<float> value) {
        style_.set_tab_bar_overline_size(value);
        return *this;
    }

    Widget& Widget::set_table_angled_headers_angle(std::optional<float> value) {
        style_.set_table_angled_headers_angle(value);
        return *this;
    }

    Widget& Widget::set_tree_lines_size(std::optional<float> value) {
        style_.set_tree_lines_size(value);
        return *this;
    }

    Widget& Widget::set_tree_lines_rounding(std::optional<float> value) {
        style_.set_tree_lines_rounding(value);
        return *this;
    }

    Widget& Widget::set_menu_item_rounding(std::optional<float> value) {
        style_.set_menu_item_rounding(value);
        return *this;
    }

    Widget& Widget::set_selectable_rounding(std::optional<float> value) {
        style_.set_selectable_rounding(value);
        return *this;
    }

    Widget& Widget::set_drag_drop_target_rounding(std::optional<float> value) {
        style_.set_drag_drop_target_rounding(value);
        return *this;
    }

    Widget& Widget::set_separator_size(std::optional<float> value) {
        style_.set_separator_size(value);
        return *this;
    }

    Widget& Widget::set_separator_text_border_size(std::optional<float> value) {
        style_.set_separator_text_border_size(value);
        return *this;
    }

    Widget& Widget::set_docking_separator_size(std::optional<float> value) {
        style_.set_docking_separator_size(value);
        return *this;
    }

    Widget& Widget::set_window_padding(std::optional<ImVec2> value) {
        style_.set_window_padding(value);
        return *this;
    }

    Widget& Widget::set_window_min_size(std::optional<ImVec2> value) {
        style_.set_window_min_size(value);
        return *this;
    }

    Widget& Widget::set_window_title_align(std::optional<ImVec2> value) {
        style_.set_window_title_align(value);
        return *this;
    }

    Widget& Widget::set_frame_padding(std::optional<ImVec2> value) {
        style_.set_frame_padding(value);
        return *this;
    }

    Widget& Widget::set_item_spacing(std::optional<ImVec2> value) {
        style_.set_item_spacing(value);
        return *this;
    }

    Widget& Widget::set_item_inner_spacing(std::optional<ImVec2> value) {
        style_.set_item_inner_spacing(value);
        return *this;
    }

    Widget& Widget::set_cell_padding(std::optional<ImVec2> value) {
        style_.set_cell_padding(value);
        return *this;
    }

    Widget& Widget::set_table_angled_headers_text_align(std::optional<ImVec2> value) {
        style_.set_table_angled_headers_text_align(value);
        return *this;
    }

    Widget& Widget::set_button_text_align(std::optional<ImVec2> value) {
        style_.set_button_text_align(value);
        return *this;
    }

    Widget& Widget::set_selectable_text_align(std::optional<ImVec2> value) {
        style_.set_selectable_text_align(value);
        return *this;
    }

    Widget& Widget::set_separator_text_align(std::optional<ImVec2> value) {
        style_.set_separator_text_align(value);
        return *this;
    }

    Widget& Widget::set_separator_text_padding(std::optional<ImVec2> value) {
        style_.set_separator_text_padding(value);
        return *this;
    }

    std::optional<ImVec4> Widget::color(ImGuiCol idx) const {
        return style_.color(idx);
    }

    ImFont* Widget::font() const {
        return style_.font();
    }

    float Widget::font_size() const {
        return style_.font_size();
    }

    std::optional<float> Widget::alpha() const {
        return style_.alpha();
    }

    std::optional<float> Widget::disabled_alpha() const {
        return style_.disabled_alpha();
    }

    std::optional<float> Widget::window_rounding() const {
        return style_.window_rounding();
    }

    std::optional<float> Widget::window_border_size() const {
        return style_.window_border_size();
    }

    std::optional<float> Widget::child_rounding() const {
        return style_.child_rounding();
    }

    std::optional<float> Widget::child_border_size() const {
        return style_.child_border_size();
    }

    std::optional<float> Widget::popup_rounding() const {
        return style_.popup_rounding();
    }

    std::optional<float> Widget::popup_border_size() const {
        return style_.popup_border_size();
    }

    std::optional<float> Widget::frame_rounding() const {
        return style_.frame_rounding();
    }

    std::optional<float> Widget::frame_border_size() const {
        return style_.frame_border_size();
    }

    std::optional<float> Widget::indent_spacing() const {
        return style_.indent_spacing();
    }

    std::optional<float> Widget::scrollbar_size() const {
        return style_.scrollbar_size();
    }

    std::optional<float> Widget::scrollbar_rounding() const {
        return style_.scrollbar_rounding();
    }

    std::optional<float> Widget::scrollbar_padding() const {
        return style_.scrollbar_padding();
    }

    std::optional<float> Widget::grab_min_size() const {
        return style_.grab_min_size();
    }

    std::optional<float> Widget::grab_rounding() const {
        return style_.grab_rounding();
    }

    std::optional<float> Widget::image_rounding() const {
        return style_.image_rounding();
    }

    std::optional<float> Widget::image_border_size() const {
        return style_.image_border_size();
    }

    std::optional<float> Widget::tab_rounding() const {
        return style_.tab_rounding();
    }

    std::optional<float> Widget::tab_border_size() const {
        return style_.tab_border_size();
    }

    std::optional<float> Widget::tab_min_width_base() const {
        return style_.tab_min_width_base();
    }

    std::optional<float> Widget::tab_min_width_shrink() const {
        return style_.tab_min_width_shrink();
    }

    std::optional<float> Widget::tab_bar_border_size() const {
        return style_.tab_bar_border_size();
    }

    std::optional<float> Widget::tab_bar_overline_size() const {
        return style_.tab_bar_overline_size();
    }

    std::optional<float> Widget::table_angled_headers_angle() const {
        return style_.table_angled_headers_angle();
    }

    std::optional<float> Widget::tree_lines_size() const {
        return style_.tree_lines_size();
    }

    std::optional<float> Widget::tree_lines_rounding() const {
        return style_.tree_lines_rounding();
    }

    std::optional<float> Widget::menu_item_rounding() const {
        return style_.menu_item_rounding();
    }

    std::optional<float> Widget::selectable_rounding() const {
        return style_.selectable_rounding();
    }

    std::optional<float> Widget::drag_drop_target_rounding() const {
        return style_.drag_drop_target_rounding();
    }

    std::optional<float> Widget::separator_size() const {
        return style_.separator_size();
    }

    std::optional<float> Widget::separator_text_border_size() const {
        return style_.separator_text_border_size();
    }

    std::optional<float> Widget::docking_separator_size() const {
        return style_.docking_separator_size();
    }

    std::optional<ImVec2> Widget::window_padding() const {
        return style_.window_padding();
    }

    std::optional<ImVec2> Widget::window_min_size() const {
        return style_.window_min_size();
    }

    std::optional<ImVec2> Widget::window_title_align() const {
        return style_.window_title_align();
    }

    std::optional<ImVec2> Widget::frame_padding() const {
        return style_.frame_padding();
    }

    std::optional<ImVec2> Widget::item_spacing() const {
        return style_.item_spacing();
    }

    std::optional<ImVec2> Widget::item_inner_spacing() const {
        return style_.item_inner_spacing();
    }

    std::optional<ImVec2> Widget::cell_padding() const {
        return style_.cell_padding();
    }

    std::optional<ImVec2> Widget::table_angled_headers_text_align() const {
        return style_.table_angled_headers_text_align();
    }

    std::optional<ImVec2> Widget::button_text_align() const {
        return style_.button_text_align();
    }

    std::optional<ImVec2> Widget::selectable_text_align() const {
        return style_.selectable_text_align();
    }

    std::optional<ImVec2> Widget::separator_text_align() const {
        return style_.separator_text_align();
    }

    std::optional<ImVec2> Widget::separator_text_padding() const {
        return style_.separator_text_padding();
    }
}
