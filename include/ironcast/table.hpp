#pragma once

#include <string>
#include <variant>
#include <vector>

#include "ironcast/container.hpp"

namespace ironcast {
    // A grid of cells with a header row. Yoga places and sizes the table (full parent width
    // by default, height from the rows); the cells are drawn in ImGui's table flow and are
    // never added to the Yoga tree.
    // A cell is a plain number or a widget (use Text for strings). Widget cells are owned by
    // the protected Container; cells_ points into it.
    class Table : public Widget, protected Container<Widget> {
        using Cell = std::variant<int, float, Widget*>;

        // How ImGui sizes a column. Auto: ImGui's default for the table's sizing policy.
        struct ColumnSizing {
            ImGuiTableColumnFlags flags = ImGuiTableColumnFlags_None;
            // Width in points (WidthFixed) or weight (WidthStretch); 0 = ImGui's default.
            float value = 0.0f;
        };

        std::vector<std::string> headers_;
        std::vector<ColumnSizing> sizing_;
        bool showHeaders_ = true;
        std::vector<Cell> cells_;
        ImGuiTableFlags flags_ = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;

        // Draws one cell at ImGui's current table cell.
        bool draw_cell(Cell& cell);
        // One cell's content height, cell padding not included.
        float cell_height(Cell& cell);

    public:
        // One column per header.
        Table(std::vector<std::string> headers, Widget* parent = nullptr);
        // columns columns and no header row.
        Table(size_t columns, Widget* parent = nullptr);

        // Cells fill the table row by row.
        Table& add_cell(int value);
        Table& add_cell(float value);

        // Returned reference stays valid for the Table's lifetime.
        template <WidgetConcept W, class... Args>
        W& add_cell(Args&&... args) {
            auto& cell = add<W>(std::forward<Args>(args)...);
            cell.set_entry_owner(*this);
            cells_.emplace_back(&cell);
            layout().mark_dirty();
            return cell;
        }

        [[nodiscard]] size_t count() const;
        [[nodiscard]] size_t columns() const;
        [[nodiscard]] size_t rows() const;

        // Column index always width points wide, cell padding not included.
        Table& set_column_width(size_t index, float width);
        // Column index shares the width left after fixed columns, in proportion to weight.
        Table& set_column_stretch(size_t index, float weight = 1.0f);
        // With ImGuiTableFlags_Resizable both are only the starting widths; the user drags from there.

        // With ImGuiTableFlags_ScrollY the table scrolls inside its own box: give it a height (height() or
        // flex), otherwise it shows only the header and the first row. Only the visible rows are drawn, so
        // large tables stay cheap; widget cells scrolled out of view don't run draw(). The header row stays
        // on top.
        Table& set_flags(ImGuiTableFlags flags);
        [[nodiscard]] ImGuiTableFlags flags() const;

        bool show() override;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) override;
    };
}
