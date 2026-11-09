
#pragma once

#include <string>
#include <functional>

#include "ironcast/widget.hpp"
#include "ironcast/container.hpp"
#include "ironcast/radiobutton.hpp"

namespace ironcast {
    // A set of RadioButtons of which exactly one is selected, by index; the first one at the start.
    // Options are its Yoga children, in a column by default; row() lays them out side by side.
    // Container is protected: options are added through add_option(), which links them to selected_.
    class RadioGroup : public Widget, protected Container<RadioButton> {
        int selected_ = 0;

    public:
        using Container::count;
        using Container::at;
        using Container::items;

        RadioGroup(Widget* parent = nullptr);

        // Options hold a pointer to selected_, so moves re-point them at the new group.
        RadioGroup(RadioGroup&& other) noexcept;
        RadioGroup& operator=(RadioGroup&& other) noexcept;

        // Returned reference stays valid for the RadioGroup's lifetime.
        RadioButton& add_option(const std::string& label);
        RadioButton& add_option(const std::string& label, std::function<void()> onSelect);
        // Removes and destroys the option at index; later options move down by one, so values stay equal to
        // indices. The same option stays selected; if it was the removed one, the first option is. Not from
        // inside a draw.
        void remove_option(size_t index);

        // Index of the selected option. set_selected() doesn't call the option's on_select.
        [[nodiscard]] int selected() const;
        void set_selected(int index);

        bool show() override;
    };
}
