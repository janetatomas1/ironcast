
#include "ironcast/assert.hpp"

#include "ironcast/radiogroup.hpp"

using namespace ironcast::literals;

namespace ironcast {
    RadioGroup::RadioGroup(Widget* parent)
    : Widget("", parent) {
        column();
        gap(0.5_pcts);
    }

    RadioGroup::RadioGroup(RadioGroup&& other) noexcept
    : Widget(std::move(other)),
    Container(std::move(other)),
    selected_(other.selected_) {
        for (auto& option : items()) {
            option.groupValue_ = &selected_;
        }
    }

    RadioGroup& RadioGroup::operator=(RadioGroup&& other) noexcept {
        if (this != &other) {
            Widget::operator=(std::move(other));
            selected_ = other.selected_;
            Container::operator=(std::move(other));

            for (auto& option : items()) {
                option.groupValue_ = &selected_;
            }
        }

        return *this;
    }

    RadioButton& RadioGroup::add_option(const std::string& label) {
        auto& rb = add(label, &selected_, static_cast<int>(count()), this);
        rb.margin(10);
        return rb;
    }

    RadioButton& RadioGroup::add_option(const std::string& label, std::function<void()> onSelect) {
        auto& rb = add(
            label, &selected_, static_cast<int>(count()), std::move(onSelect), this
        );
        rb.margin(10);
        return rb;
    }

    void RadioGroup::remove_option(size_t index) {
        IRONCAST_DEBUG_ASSERT(index < count(), "RadioGroup::remove_option index out of range", index, count());
        remove(index);

        int value = 0;
        for (auto& option : items()) {
            option.value_ = value++;
        }

        const int removed = static_cast<int>(index);
        if (selected_ > removed) {
            selected_ -= 1;
        } else if (selected_ == removed) {
            selected_ = 0;
        }
    }

    int RadioGroup::selected() const {
        return selected_;
    }

    void RadioGroup::set_selected(int index) {
        IRONCAST_DEBUG_ASSERT(index >= 0 && static_cast<size_t>(index) < count(),
                     "RadioGroup::set_selected index out of range", index, count());
        selected_ = index;
    }

    bool RadioGroup::show() {
        int before = selected_;
        Widget::show();
        return selected_ != before;
    }
}
