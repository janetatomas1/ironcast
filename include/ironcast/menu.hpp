
#pragma once

#include <string>

#include <function2/function2.hpp>

#include "ironcast/container.hpp"

namespace ironcast {
    // Entries of a Menu are drawn in ImGui's popup flow, not placed by Yoga:
    // they are owned by the Menu and never added to the Yoga tree.

    // A clickable entry of a Menu, with an optional shortcut text and check mark.
    class MenuItem : public Widget {
        std::string shortcut_;
        fu2::unique_function<void()> callback_;
        bool checkable_ = false;
        bool checked_ = false;

    public:
        MenuItem(
            const std::string& title,
            fu2::unique_function<void()> callback = nullptr,
            const std::string& shortcut = ""
        );

        // The shortcut is only displayed; ImGui does not bind it.
        MenuItem& set_shortcut(const std::string& shortcut);
        [[nodiscard]] const std::string& shortcut() const;

        // Called on the next dispatch after a click (see post()). Empty = none.
        MenuItem& set_callback(fu2::unique_function<void()> callback);

        // A checkable item toggles checked() when clicked, before the callback runs.
        MenuItem& set_checkable(bool checkable);
        [[nodiscard]] bool checkable() const;
        MenuItem& set_checked(bool checked);
        [[nodiscard]] bool checked() const;

        bool show() final;
    };

    // A horizontal line between entries of a Menu; added with Menu::add_separator().
    class MenuSeparator : public Widget {
    public:
        bool show() final;
    };

    // A menu header in a MenuBar, or a submenu in another Menu, opening a list of entries.
    // Container is protected: entries are added through add_item(), add_menu() and add_separator().
    class Menu : public Widget, protected Container<Widget> {
    public:
        using Container::count;
        using Container::at;
        using Container::items;

        Menu(const std::string& title);

        // Returned references stay valid for the Menu's lifetime.
        MenuItem& add_item(
            const std::string& title,
            fu2::unique_function<void()> callback = nullptr,
            const std::string& shortcut = ""
        );
        Menu& add_menu(const std::string& title);
        Menu& add_separator();

        // Draws the menu header and, while open, its entries. Must be called inside a
        // menu bar or another open menu.
        bool show() override;
    };
}
