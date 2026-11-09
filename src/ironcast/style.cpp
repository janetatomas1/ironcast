#include <algorithm>

#include "ironcast/style.hpp"
#include "ironcast/widget.hpp"

namespace ironcast {
    namespace {
        template<typename T>
        std::optional<T> get(const std::vector<std::pair<int, T>>& entries, int idx) {
            const auto it = std::ranges::find(entries, idx, &std::pair<int, T>::first);
            return it != entries.end() ? std::optional<T>{it->second} : std::nullopt;
        }

        // Overwrites idx's entry, adds one if it has none, or removes it for std::nullopt.
        template<typename T>
        void set(std::vector<std::pair<int, T>>& entries, int idx, std::optional<T> value) {
            const auto it = std::ranges::find(entries, idx, &std::pair<int, T>::first);

            if (!value) {
                if (it != entries.end()) {
                    entries.erase(it);
                }
            } else if (it != entries.end()) {
                it->second = *value;
            } else {
                entries.emplace_back(idx, *value);
            }
        }
    }

    Style::Style(Widget* widget): widget_(widget) {
    }

    void Style::set_widget(Widget* widget) {
        widget_ = widget;
    }

    void Style::mark_dirty() const {
        if (widget_ != nullptr) {
            widget_->layout().mark_dirty_tree();
        }
    }

    void Style::push() const {
        for (const auto& [idx, color] : colors_) {
            ImGui::PushStyleColor(idx, color);
        }

        for (const auto& [idx, value] : floats_) {
            ImGui::PushStyleVar(idx, value);
        }

        for (const auto& [idx, value] : vec2s_) {
            ImGui::PushStyleVar(idx, value);
        }

        if (font_ || fontSize_ > 0.0f) {
            // nullptr keeps the current font, 0 keeps the current size
            ImGui::PushFont(font_, fontSize_);
        }
    }

    void Style::pop() const {
        ImGui::PopStyleColor(static_cast<int>(colors_.size()));
        ImGui::PopStyleVar(static_cast<int>(floats_.size() + vec2s_.size()));

        if (font_ || fontSize_ > 0.0f) {
            ImGui::PopFont();
        }
    }

    std::optional<ImVec4> Style::color(ImGuiCol idx) const {
        return get(colors_, idx);
    }

    Style& Style::color(ImGuiCol idx, std::optional<ImVec4> color) {
        set(colors_, idx, color);
        return *this;
    }

    ImFont* Style::font() const {
        return font_;
    }

    Style& Style::set_font(ImFont* font) {
        font_ = font;
        mark_dirty();
        return *this;
    }

    float Style::font_size() const {
        return fontSize_;
    }

    Style& Style::set_font_size(float value) {
        fontSize_ = value;
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::alpha() const {
        return get(floats_, ImGuiStyleVar_Alpha);
    }

    Style& Style::set_alpha(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_Alpha, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::disabled_alpha() const {
        return get(floats_, ImGuiStyleVar_DisabledAlpha);
    }

    Style& Style::set_disabled_alpha(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_DisabledAlpha, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::window_rounding() const {
        return get(floats_, ImGuiStyleVar_WindowRounding);
    }

    Style& Style::set_window_rounding(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_WindowRounding, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::window_border_size() const {
        return get(floats_, ImGuiStyleVar_WindowBorderSize);
    }

    Style& Style::set_window_border_size(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_WindowBorderSize, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::child_rounding() const {
        return get(floats_, ImGuiStyleVar_ChildRounding);
    }

    Style& Style::set_child_rounding(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_ChildRounding, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::child_border_size() const {
        return get(floats_, ImGuiStyleVar_ChildBorderSize);
    }

    Style& Style::set_child_border_size(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_ChildBorderSize, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::popup_rounding() const {
        return get(floats_, ImGuiStyleVar_PopupRounding);
    }

    Style& Style::set_popup_rounding(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_PopupRounding, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::popup_border_size() const {
        return get(floats_, ImGuiStyleVar_PopupBorderSize);
    }

    Style& Style::set_popup_border_size(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_PopupBorderSize, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::frame_rounding() const {
        return get(floats_, ImGuiStyleVar_FrameRounding);
    }

    Style& Style::set_frame_rounding(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_FrameRounding, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::frame_border_size() const {
        return get(floats_, ImGuiStyleVar_FrameBorderSize);
    }

    Style& Style::set_frame_border_size(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_FrameBorderSize, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::indent_spacing() const {
        return get(floats_, ImGuiStyleVar_IndentSpacing);
    }

    Style& Style::set_indent_spacing(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_IndentSpacing, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::scrollbar_size() const {
        return get(floats_, ImGuiStyleVar_ScrollbarSize);
    }

    Style& Style::set_scrollbar_size(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_ScrollbarSize, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::scrollbar_rounding() const {
        return get(floats_, ImGuiStyleVar_ScrollbarRounding);
    }

    Style& Style::set_scrollbar_rounding(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_ScrollbarRounding, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::scrollbar_padding() const {
        return get(floats_, ImGuiStyleVar_ScrollbarPadding);
    }

    Style& Style::set_scrollbar_padding(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_ScrollbarPadding, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::grab_min_size() const {
        return get(floats_, ImGuiStyleVar_GrabMinSize);
    }

    Style& Style::set_grab_min_size(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_GrabMinSize, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::grab_rounding() const {
        return get(floats_, ImGuiStyleVar_GrabRounding);
    }

    Style& Style::set_grab_rounding(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_GrabRounding, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::image_rounding() const {
        return get(floats_, ImGuiStyleVar_ImageRounding);
    }

    Style& Style::set_image_rounding(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_ImageRounding, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::image_border_size() const {
        return get(floats_, ImGuiStyleVar_ImageBorderSize);
    }

    Style& Style::set_image_border_size(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_ImageBorderSize, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::tab_rounding() const {
        return get(floats_, ImGuiStyleVar_TabRounding);
    }

    Style& Style::set_tab_rounding(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_TabRounding, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::tab_border_size() const {
        return get(floats_, ImGuiStyleVar_TabBorderSize);
    }

    Style& Style::set_tab_border_size(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_TabBorderSize, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::tab_min_width_base() const {
        return get(floats_, ImGuiStyleVar_TabMinWidthBase);
    }

    Style& Style::set_tab_min_width_base(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_TabMinWidthBase, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::tab_min_width_shrink() const {
        return get(floats_, ImGuiStyleVar_TabMinWidthShrink);
    }

    Style& Style::set_tab_min_width_shrink(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_TabMinWidthShrink, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::tab_bar_border_size() const {
        return get(floats_, ImGuiStyleVar_TabBarBorderSize);
    }

    Style& Style::set_tab_bar_border_size(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_TabBarBorderSize, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::tab_bar_overline_size() const {
        return get(floats_, ImGuiStyleVar_TabBarOverlineSize);
    }

    Style& Style::set_tab_bar_overline_size(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_TabBarOverlineSize, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::table_angled_headers_angle() const {
        return get(floats_, ImGuiStyleVar_TableAngledHeadersAngle);
    }

    Style& Style::set_table_angled_headers_angle(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_TableAngledHeadersAngle, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::tree_lines_size() const {
        return get(floats_, ImGuiStyleVar_TreeLinesSize);
    }

    Style& Style::set_tree_lines_size(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_TreeLinesSize, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::tree_lines_rounding() const {
        return get(floats_, ImGuiStyleVar_TreeLinesRounding);
    }

    Style& Style::set_tree_lines_rounding(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_TreeLinesRounding, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::menu_item_rounding() const {
        return get(floats_, ImGuiStyleVar_MenuItemRounding);
    }

    Style& Style::set_menu_item_rounding(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_MenuItemRounding, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::selectable_rounding() const {
        return get(floats_, ImGuiStyleVar_SelectableRounding);
    }

    Style& Style::set_selectable_rounding(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_SelectableRounding, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::drag_drop_target_rounding() const {
        return get(floats_, ImGuiStyleVar_DragDropTargetRounding);
    }

    Style& Style::set_drag_drop_target_rounding(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_DragDropTargetRounding, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::separator_size() const {
        return get(floats_, ImGuiStyleVar_SeparatorSize);
    }

    Style& Style::set_separator_size(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_SeparatorSize, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::separator_text_border_size() const {
        return get(floats_, ImGuiStyleVar_SeparatorTextBorderSize);
    }

    Style& Style::set_separator_text_border_size(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_SeparatorTextBorderSize, value);
        mark_dirty();
        return *this;
    }

    std::optional<float> Style::docking_separator_size() const {
        return get(floats_, ImGuiStyleVar_DockingSeparatorSize);
    }

    Style& Style::set_docking_separator_size(std::optional<float> value) {
        set(floats_, ImGuiStyleVar_DockingSeparatorSize, value);
        mark_dirty();
        return *this;
    }

    std::optional<ImVec2> Style::window_padding() const {
        return get(vec2s_, ImGuiStyleVar_WindowPadding);
    }

    Style& Style::set_window_padding(std::optional<ImVec2> value) {
        set(vec2s_, ImGuiStyleVar_WindowPadding, value);
        mark_dirty();
        return *this;
    }

    std::optional<ImVec2> Style::window_min_size() const {
        return get(vec2s_, ImGuiStyleVar_WindowMinSize);
    }

    Style& Style::set_window_min_size(std::optional<ImVec2> value) {
        set(vec2s_, ImGuiStyleVar_WindowMinSize, value);
        mark_dirty();
        return *this;
    }

    std::optional<ImVec2> Style::window_title_align() const {
        return get(vec2s_, ImGuiStyleVar_WindowTitleAlign);
    }

    Style& Style::set_window_title_align(std::optional<ImVec2> value) {
        set(vec2s_, ImGuiStyleVar_WindowTitleAlign, value);
        mark_dirty();
        return *this;
    }

    std::optional<ImVec2> Style::frame_padding() const {
        return get(vec2s_, ImGuiStyleVar_FramePadding);
    }

    Style& Style::set_frame_padding(std::optional<ImVec2> value) {
        set(vec2s_, ImGuiStyleVar_FramePadding, value);
        mark_dirty();
        return *this;
    }

    std::optional<ImVec2> Style::item_spacing() const {
        return get(vec2s_, ImGuiStyleVar_ItemSpacing);
    }

    Style& Style::set_item_spacing(std::optional<ImVec2> value) {
        set(vec2s_, ImGuiStyleVar_ItemSpacing, value);
        mark_dirty();
        return *this;
    }

    std::optional<ImVec2> Style::item_inner_spacing() const {
        return get(vec2s_, ImGuiStyleVar_ItemInnerSpacing);
    }

    Style& Style::set_item_inner_spacing(std::optional<ImVec2> value) {
        set(vec2s_, ImGuiStyleVar_ItemInnerSpacing, value);
        mark_dirty();
        return *this;
    }

    std::optional<ImVec2> Style::cell_padding() const {
        return get(vec2s_, ImGuiStyleVar_CellPadding);
    }

    Style& Style::set_cell_padding(std::optional<ImVec2> value) {
        set(vec2s_, ImGuiStyleVar_CellPadding, value);
        mark_dirty();
        return *this;
    }

    std::optional<ImVec2> Style::table_angled_headers_text_align() const {
        return get(vec2s_, ImGuiStyleVar_TableAngledHeadersTextAlign);
    }

    Style& Style::set_table_angled_headers_text_align(std::optional<ImVec2> value) {
        set(vec2s_, ImGuiStyleVar_TableAngledHeadersTextAlign, value);
        mark_dirty();
        return *this;
    }

    std::optional<ImVec2> Style::button_text_align() const {
        return get(vec2s_, ImGuiStyleVar_ButtonTextAlign);
    }

    Style& Style::set_button_text_align(std::optional<ImVec2> value) {
        set(vec2s_, ImGuiStyleVar_ButtonTextAlign, value);
        mark_dirty();
        return *this;
    }

    std::optional<ImVec2> Style::selectable_text_align() const {
        return get(vec2s_, ImGuiStyleVar_SelectableTextAlign);
    }

    Style& Style::set_selectable_text_align(std::optional<ImVec2> value) {
        set(vec2s_, ImGuiStyleVar_SelectableTextAlign, value);
        mark_dirty();
        return *this;
    }

    std::optional<ImVec2> Style::separator_text_align() const {
        return get(vec2s_, ImGuiStyleVar_SeparatorTextAlign);
    }

    Style& Style::set_separator_text_align(std::optional<ImVec2> value) {
        set(vec2s_, ImGuiStyleVar_SeparatorTextAlign, value);
        mark_dirty();
        return *this;
    }

    std::optional<ImVec2> Style::separator_text_padding() const {
        return get(vec2s_, ImGuiStyleVar_SeparatorTextPadding);
    }

    Style& Style::set_separator_text_padding(std::optional<ImVec2> value) {
        set(vec2s_, ImGuiStyleVar_SeparatorTextPadding, value);
        mark_dirty();
        return *this;
    }
}
