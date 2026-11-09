
#pragma once

#include <string>
#include <vector>
#include <functional>

#include "ironcast/widget.hpp"

namespace ironcast {
    using DropdownFlags = ImGuiComboFlags;

    // A combo box over a list of string options, with the title as its label. Nothing is selected
    // at first (index -1), and the closed combo shows the placeholder.
    class Dropdown : public Widget {
        std::vector<std::string> options_;
        int selected_ = -1;
        std::function<void(int)> onChange_;
        std::string placeholder_ = "Select...";
        DropdownFlags flags_ = ImGuiComboFlags_None;

        // Text shown in the closed combo: the selected option, or the placeholder.
        [[nodiscard]] const char* preview() const;

    public:
        Dropdown(const std::string& title = "", Widget* parent = nullptr);

        Dropdown& add_option(const std::string& label);
        Dropdown& insert_option(size_t index, const std::string& label);
        // Removing the selected option clears the selection; later indices shift down.
        void remove_option(size_t index);
        void clear_options();

        // Index of the selected option, -1 for none. set_selected() doesn't call on_change.
        [[nodiscard]] int selected() const;
        void set_selected(int index);

        // Requires has_selection().
        [[nodiscard]] const std::string& selected_label() const;
        [[nodiscard]] bool has_selection() const;

        [[nodiscard]] size_t count() const;
        [[nodiscard]] const std::string& at(size_t index) const;

        Dropdown& set_placeholder(const std::string& text);
        [[nodiscard]] const std::string& placeholder() const;

        [[nodiscard]] DropdownFlags flags() const;
        Dropdown& set_flags(DropdownFlags flags);

        // Called on the next dispatch (see post()) with the new index after the user picks a
        // different option.
        Dropdown& set_on_change(std::function<void(int)> callback);

        // True on the frame the user picks a different option.
        bool show() final;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) final;
    };
}
