#pragma once

#include <function2/function2.hpp>
#include <string>
#include <vector>

#include "ironcast/widget.hpp"

namespace ironcast {
    // A list of selectable rows with ImGui multi-selection: click selects one, ctrl+click
    // toggles, shift+click selects a range, ctrl+A selects all. Flags change that, e.g.
    // ImGuiMultiSelectFlags_SingleSelect or _BoxSelect1d.
    // Rows are one text line tall, packed without gaps, and as wide as the Yoga box. The
    // highlight fills exactly each row: ImGui's half-ItemSpacing padding is off.
    class Selectable : public Widget {
        std::vector<std::string> labels_;
        std::vector<bool> selected_;
        // ScopeRect: box-select and click-void clearing stay inside this list, not the whole window.
        ImGuiMultiSelectFlags flags_ = ImGuiMultiSelectFlags_ScopeRect;
        fu2::unique_function<void(std::vector<size_t>)> callback_;

    public:
        // The callback gets the selected indices, in order.
        Selectable(fu2::unique_function<void(std::vector<size_t>)> callback = nullptr, Widget* parent = nullptr);

        // Both apply on the next dispatch (count() doesn't change until then). remove_item's index
        // is resolved then, so queued adds and removes apply in call order.
        Selectable& add_item(const std::string& label);
        Selectable& remove_item(size_t index);
        [[nodiscard]] size_t count() const;
        [[nodiscard]] const std::string& label(size_t index) const;

        [[nodiscard]] bool is_selected(size_t index) const;
        Selectable& set_selected(size_t index, bool selected);
        // The selected indices, in order.
        [[nodiscard]] std::vector<size_t> selected() const;

        Selectable& set_flags(ImGuiMultiSelectFlags flags);
        [[nodiscard]] ImGuiMultiSelectFlags flags() const;

        Selectable& set_callback(fu2::unique_function<void(std::vector<size_t>)> callback);
        // Empty when none is set.
        [[nodiscard]] const fu2::unique_function<void(std::vector<size_t>)>& callback() const;

        bool show() final;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) final;
    };
}
