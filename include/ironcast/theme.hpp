#pragma once

#include <imgui.h>

#include "ironcast/font.hpp"

namespace ironcast {
    // Font size used when a theme leaves style.FontSizeBase at 0 (ImGuiStyle's default): 1.5x ImGui's 13px.
    inline constexpr float defaultFontSize = 19.5f;

    // App-wide look, copied into every window's ImGui context when it is created.
    // Sizes are unscaled; each window scales them for its display. Defaults are ImGui's dark style.
    // Charts have their own ChartTheme (ironcast/extras/charttheme.hpp).
    struct Theme {
        ImGuiStyle style;
        // Each window's default font, replacing Window::set_font() when the theme is applied; its size is
        // style.FontSizeBase. Borrowed: from Ironcast::add_font() or a bundled Font::inter() etc.
        // nullptr = ImGui's built-in font.
        const Font* font = nullptr;

        // ImGui's built-in colour presets.
        [[nodiscard]] static Theme dark();
        [[nodiscard]] static Theme light();
        [[nodiscard]] static Theme classic();
        // Phosphor green on black, square corners, thin borders.
        [[nodiscard]] static Theme matrix();
        // Arctic Nord palette: snow text, frost accents on polar-night greys, soft corners.
        [[nodiscard]] static Theme nord();
        // White lines on blueprint blue, square corners, borders everywhere.
        [[nodiscard]] static Theme blueprint();
        // Atom's One Dark: blue accent on blue-grey, soft corners.
        [[nodiscard]] static Theme one_dark();
    };
}
