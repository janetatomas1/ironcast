#pragma once

#include <optional>
#include <utility>
#include <vector>

#include <imgui.h>

namespace ironcast {
    class Widget;

    // ImGui style overrides: only the colours and vars that were set, each kept with its ImGui
    // index. Unset (std::nullopt) = whatever ImGui's style is when pushed.
    class Style {
        Widget* widget_ = nullptr;

        std::vector<std::pair<ImGuiCol, ImVec4>> colors_;
        std::vector<std::pair<ImGuiStyleVar, float>> floats_;
        std::vector<std::pair<ImGuiStyleVar, ImVec2>> vec2s_;
        ImFont* font_ = nullptr;
        float fontSize_ = 0.0f;

        // Style vars change measured sizes, for the widget and everything inside it.
        void mark_dirty() const;

    public:
        // widget is re-measured when a style var changes; may be nullptr.
        Style(Widget* widget = nullptr);

        Style(const Style& other) = delete;
        Style& operator=(const Style& other) = delete;

        Style(Style&& other) noexcept = default;
        Style& operator=(Style&& other) noexcept = default;

        void set_widget(Widget* widget);

        // Pushes every set value onto ImGui's style stacks; pop() takes them off again.
        void push() const;
        void pop() const;

        [[nodiscard]] std::optional<ImVec4> color(ImGuiCol idx) const;
        Style& color(ImGuiCol idx, std::optional<ImVec4> color);

        // Belongs to the current ImGui context's atlas. nullptr = keep the current font.
        [[nodiscard]] ImFont* font() const;
        Style& set_font(ImFont* font);

        // Unscaled size, like ImGuiStyle::FontSizeBase; global scaling still applies on top.
        // 0 = keep the current size.
        [[nodiscard]] float font_size() const;
        Style& set_font_size(float value);

        [[nodiscard]] std::optional<float> alpha() const;
        Style& set_alpha(std::optional<float> value);

        [[nodiscard]] std::optional<float> disabled_alpha() const;
        Style& set_disabled_alpha(std::optional<float> value);

        [[nodiscard]] std::optional<float> window_rounding() const;
        Style& set_window_rounding(std::optional<float> value);

        [[nodiscard]] std::optional<float> window_border_size() const;
        Style& set_window_border_size(std::optional<float> value);

        [[nodiscard]] std::optional<float> child_rounding() const;
        Style& set_child_rounding(std::optional<float> value);

        [[nodiscard]] std::optional<float> child_border_size() const;
        Style& set_child_border_size(std::optional<float> value);

        [[nodiscard]] std::optional<float> popup_rounding() const;
        Style& set_popup_rounding(std::optional<float> value);

        [[nodiscard]] std::optional<float> popup_border_size() const;
        Style& set_popup_border_size(std::optional<float> value);

        [[nodiscard]] std::optional<float> frame_rounding() const;
        Style& set_frame_rounding(std::optional<float> value);

        [[nodiscard]] std::optional<float> frame_border_size() const;
        Style& set_frame_border_size(std::optional<float> value);

        [[nodiscard]] std::optional<float> indent_spacing() const;
        Style& set_indent_spacing(std::optional<float> value);

        [[nodiscard]] std::optional<float> scrollbar_size() const;
        Style& set_scrollbar_size(std::optional<float> value);

        [[nodiscard]] std::optional<float> scrollbar_rounding() const;
        Style& set_scrollbar_rounding(std::optional<float> value);

        [[nodiscard]] std::optional<float> scrollbar_padding() const;
        Style& set_scrollbar_padding(std::optional<float> value);

        [[nodiscard]] std::optional<float> grab_min_size() const;
        Style& set_grab_min_size(std::optional<float> value);

        [[nodiscard]] std::optional<float> grab_rounding() const;
        Style& set_grab_rounding(std::optional<float> value);

        [[nodiscard]] std::optional<float> image_rounding() const;
        Style& set_image_rounding(std::optional<float> value);

        [[nodiscard]] std::optional<float> image_border_size() const;
        Style& set_image_border_size(std::optional<float> value);

        [[nodiscard]] std::optional<float> tab_rounding() const;
        Style& set_tab_rounding(std::optional<float> value);

        [[nodiscard]] std::optional<float> tab_border_size() const;
        Style& set_tab_border_size(std::optional<float> value);

        [[nodiscard]] std::optional<float> tab_min_width_base() const;
        Style& set_tab_min_width_base(std::optional<float> value);

        [[nodiscard]] std::optional<float> tab_min_width_shrink() const;
        Style& set_tab_min_width_shrink(std::optional<float> value);

        [[nodiscard]] std::optional<float> tab_bar_border_size() const;
        Style& set_tab_bar_border_size(std::optional<float> value);

        [[nodiscard]] std::optional<float> tab_bar_overline_size() const;
        Style& set_tab_bar_overline_size(std::optional<float> value);

        [[nodiscard]] std::optional<float> table_angled_headers_angle() const;
        Style& set_table_angled_headers_angle(std::optional<float> value);

        [[nodiscard]] std::optional<float> tree_lines_size() const;
        Style& set_tree_lines_size(std::optional<float> value);

        [[nodiscard]] std::optional<float> tree_lines_rounding() const;
        Style& set_tree_lines_rounding(std::optional<float> value);

        [[nodiscard]] std::optional<float> menu_item_rounding() const;
        Style& set_menu_item_rounding(std::optional<float> value);

        [[nodiscard]] std::optional<float> selectable_rounding() const;
        Style& set_selectable_rounding(std::optional<float> value);

        [[nodiscard]] std::optional<float> drag_drop_target_rounding() const;
        Style& set_drag_drop_target_rounding(std::optional<float> value);

        [[nodiscard]] std::optional<float> separator_size() const;
        Style& set_separator_size(std::optional<float> value);

        [[nodiscard]] std::optional<float> separator_text_border_size() const;
        Style& set_separator_text_border_size(std::optional<float> value);

        [[nodiscard]] std::optional<float> docking_separator_size() const;
        Style& set_docking_separator_size(std::optional<float> value);

        [[nodiscard]] std::optional<ImVec2> window_padding() const;
        Style& set_window_padding(std::optional<ImVec2> value);

        [[nodiscard]] std::optional<ImVec2> window_min_size() const;
        Style& set_window_min_size(std::optional<ImVec2> value);

        [[nodiscard]] std::optional<ImVec2> window_title_align() const;
        Style& set_window_title_align(std::optional<ImVec2> value);

        [[nodiscard]] std::optional<ImVec2> frame_padding() const;
        Style& set_frame_padding(std::optional<ImVec2> value);

        [[nodiscard]] std::optional<ImVec2> item_spacing() const;
        Style& set_item_spacing(std::optional<ImVec2> value);

        [[nodiscard]] std::optional<ImVec2> item_inner_spacing() const;
        Style& set_item_inner_spacing(std::optional<ImVec2> value);

        [[nodiscard]] std::optional<ImVec2> cell_padding() const;
        Style& set_cell_padding(std::optional<ImVec2> value);

        [[nodiscard]] std::optional<ImVec2> table_angled_headers_text_align() const;
        Style& set_table_angled_headers_text_align(std::optional<ImVec2> value);

        [[nodiscard]] std::optional<ImVec2> button_text_align() const;
        Style& set_button_text_align(std::optional<ImVec2> value);

        [[nodiscard]] std::optional<ImVec2> selectable_text_align() const;
        Style& set_selectable_text_align(std::optional<ImVec2> value);

        [[nodiscard]] std::optional<ImVec2> separator_text_align() const;
        Style& set_separator_text_align(std::optional<ImVec2> value);

        [[nodiscard]] std::optional<ImVec2> separator_text_padding() const;
        Style& set_separator_text_padding(std::optional<ImVec2> value);
    };
}
