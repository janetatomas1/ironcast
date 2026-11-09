
#include <algorithm>

#include "ironcast/menubar.hpp"

using namespace ironcast::literals;

namespace ironcast {
    MenuBar::MenuBar(Widget* parent)
    : Widget("", parent) {
        layout().enable_measure();
        width(100_pcts);
    }

    Menu& MenuBar::add_menu(const std::string& title) {
        auto& menu = add(title);
        menu.set_entry_owner(*this);
        layout().mark_dirty();
        return menu;
    }

    bool MenuBar::show() {
        bool clicked = false;

        // BeginMenuBar needs a window with ImGuiWindowFlags_MenuBar, so the bar gets its own
        // child window sized to this widget's box.
        if (ImGui::BeginChild(
            title_id().c_str(),
            size(),
            ImGuiChildFlags_None,
            ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        )) {

            if (ImGui::BeginMenuBar()) {
                for (auto& menu : items()) {
                    // Menus are outside the Yoga tree, so they reach the app through the bar's window.
                    menu.set_window(window());
                    clicked |= menu.draw();
                }

                ImGui::EndMenuBar();
            }
        }

        ImGui::EndChild();
        return clicked;
    }

    ImVec2 MenuBar::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        // Mirrors ImGui's menu bar: MenuBarOffset.x on both ends, and each header advances by
        // its label plus the doubled ItemSpacing.x BeginMenu pushes inside a menu bar.
        const ImGuiStyle& style = ImGui::GetStyle();
        float intrinsic = std::max(style.WindowPadding.x, style.ItemSpacing.x) * 2.0f;

        for (auto& menu : items()) {
            // The header is drawn inside the menu's style, ItemSpacing included; style reads the pushed values.
            menu.style().push();
            intrinsic += ImGui::CalcTextSize(menu.title_id().c_str(), nullptr, true).x + style.ItemSpacing.x * 2.0f;
            menu.style().pop();
        }

        return ImVec2{
            fit_measure(intrinsic, width, widthMode),
            fit_measure(ImGui::GetFrameHeight(), height, heightMode)
        };
    }
}
