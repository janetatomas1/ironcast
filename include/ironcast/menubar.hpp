
#pragma once

#include "ironcast/menu.hpp"

namespace ironcast {
    // A horizontal bar of menus. Yoga places and sizes the bar (full parent width by
    // default, frame height); the menus themselves are drawn in ImGui's menu bar flow.
    // Container is protected: menus are added through add_menu(), which also re-measures the bar.
    class MenuBar : public Widget, protected Container<Menu> {
    public:
        using Container::count;
        using Container::at;
        using Container::items;

        MenuBar(Widget* parent = nullptr);

        // Returned reference stays valid for the MenuBar's lifetime.
        Menu& add_menu(const std::string& title);

        bool show() override;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) override;
    };
}
