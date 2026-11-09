
#include "ironcast/menu.hpp"
#include "ironcast/ironcast.hpp"

namespace ironcast {
    MenuItem::MenuItem(
        const std::string& title,
        fu2::unique_function<void()> callback,
        const std::string& shortcut
    ) : Widget(title), shortcut_(shortcut), callback_(std::move(callback)) {}

    MenuItem& MenuItem::set_shortcut(const std::string& shortcut) {
        shortcut_ = shortcut;
        return *this;
    }

    const std::string& MenuItem::shortcut() const {
        return shortcut_;
    }

    MenuItem& MenuItem::set_callback(fu2::unique_function<void()> callback) {
        callback_ = std::move(callback);
        return *this;
    }

    MenuItem& MenuItem::set_checkable(bool checkable) {
        checkable_ = checkable;
        return *this;
    }

    bool MenuItem::checkable() const {
        return checkable_;
    }

    MenuItem& MenuItem::set_checked(bool checked) {
        checked_ = checked;
        return *this;
    }

    bool MenuItem::checked() const {
        return checked_;
    }

    bool MenuItem::show() {
        const char* shortcut = shortcut_.empty() ? nullptr : shortcut_.c_str();

        if (ImGui::MenuItem(title_id().c_str(), shortcut, checkable_ ? &checked_ : nullptr)) {
            if (callback_) {
                post([](auto& self) {
                    if (self.callback_) {
                        self.callback_();
                    }
                });
            }

            return true;
        }

        return false;
    }

    bool MenuSeparator::show() {
        ImGui::Separator();
        return false;
    }

    Menu::Menu(const std::string& title)
    : Widget(title) {}

    MenuItem& Menu::add_item(
        const std::string& title,
        fu2::unique_function<void()> callback,
        const std::string& shortcut
    ) {
        auto& item = add<MenuItem>(title, std::move(callback), shortcut);
        item.set_entry_owner(*this);
        return item;
    }

    Menu& Menu::add_menu(const std::string& title) {
        auto& menu = add<Menu>(title);
        menu.set_entry_owner(*this);
        return menu;
    }

    Menu& Menu::add_separator() {
        add<MenuSeparator>().set_entry_owner(*this);
        return *this;
    }

    bool Menu::show() {
        bool clicked = false;

        if (ImGui::BeginMenu(title_id().c_str())) {
            for (auto& entry : items()) {
                // Entries are outside the Yoga tree, so they reach the app through this menu's window.
                entry.set_window(window());
                clicked |= entry.draw();
            }

            ImGui::EndMenu();
        }

        return clicked;
    }
}
