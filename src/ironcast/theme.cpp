#include "ironcast/theme.hpp"
#include "ironcast/color.hpp"

namespace ironcast {
    namespace {
        // The handful of colours a custom theme is made of; palette() spreads them over every ImGuiCol.
        struct Palette {
            ImVec4 base;     // darkest: title bars, scrollbar track, dimmed tabs, empty dock space
            ImVec4 bg;       // window background
            ImVec4 panel;    // frames, popups, menu bar, table headers
            ImVec4 border;   // borders, separators, table and tree lines
            ImVec4 control;  // buttons, headers, grabs at rest
            ImVec4 hover;    // ... hovered
            ImVec4 active;   // ... pressed
            ImVec4 accent;   // check marks, cursors, links, overlines, focus
            ImVec4 text;
            ImVec4 dim;      // disabled text
        };

        constexpr ImVec4 with_alpha(ImVec4 color, float alpha) {
            return {color.x, color.y, color.z, alpha};
        }

        Theme palette(const Palette& p) {
            const ImVec4 glow = with_alpha(p.accent, 0.25f);
            const ImVec4 shade = from_hex(0x000000B0);

            Theme theme;
            ImVec4* c = theme.style.Colors;

            c[ImGuiCol_Text] = p.text;
            c[ImGuiCol_TextDisabled] = p.dim;
            c[ImGuiCol_WindowBg] = p.bg;
            c[ImGuiCol_ChildBg] = p.bg;
            c[ImGuiCol_PopupBg] = p.panel;
            c[ImGuiCol_Border] = p.border;
            c[ImGuiCol_BorderShadow] = colors::transparent;
            c[ImGuiCol_FrameBg] = p.panel;
            c[ImGuiCol_FrameBgHovered] = p.control;
            c[ImGuiCol_FrameBgActive] = p.hover;
            c[ImGuiCol_TitleBg] = p.base;
            c[ImGuiCol_TitleBgActive] = p.control;
            c[ImGuiCol_TitleBgCollapsed] = p.base;
            c[ImGuiCol_MenuBarBg] = p.panel;
            c[ImGuiCol_ScrollbarBg] = p.base;
            c[ImGuiCol_ScrollbarGrab] = p.control;
            c[ImGuiCol_ScrollbarGrabHovered] = p.hover;
            c[ImGuiCol_ScrollbarGrabActive] = p.active;
            c[ImGuiCol_CheckMark] = p.accent;
            c[ImGuiCol_CheckboxSelectedBg] = p.control;
            c[ImGuiCol_SliderGrab] = p.hover;
            c[ImGuiCol_SliderGrabActive] = p.accent;
            c[ImGuiCol_Button] = p.control;
            c[ImGuiCol_ButtonHovered] = p.hover;
            c[ImGuiCol_ButtonActive] = p.active;
            c[ImGuiCol_Header] = p.control;
            c[ImGuiCol_HeaderHovered] = p.hover;
            c[ImGuiCol_HeaderActive] = p.active;
            c[ImGuiCol_Separator] = p.border;
            c[ImGuiCol_SeparatorHovered] = p.hover;
            c[ImGuiCol_SeparatorActive] = p.accent;
            c[ImGuiCol_ResizeGrip] = p.control;
            c[ImGuiCol_ResizeGripHovered] = p.hover;
            c[ImGuiCol_ResizeGripActive] = p.active;
            c[ImGuiCol_InputTextCursor] = p.accent;
            c[ImGuiCol_TabHovered] = p.hover;
            c[ImGuiCol_Tab] = p.panel;
            c[ImGuiCol_TabSelected] = p.control;
            c[ImGuiCol_TabSelectedOverline] = p.accent;
            c[ImGuiCol_TabDimmed] = p.base;
            c[ImGuiCol_TabDimmedSelected] = p.panel;
            c[ImGuiCol_TabDimmedSelectedOverline] = p.hover;
            c[ImGuiCol_DockingPreview] = glow;
            c[ImGuiCol_DockingEmptyBg] = p.base;
            c[ImGuiCol_PlotLines] = p.hover;
            c[ImGuiCol_PlotLinesHovered] = p.accent;
            c[ImGuiCol_PlotHistogram] = p.hover;
            c[ImGuiCol_PlotHistogramHovered] = p.accent;
            c[ImGuiCol_TableHeaderBg] = p.panel;
            c[ImGuiCol_TableBorderStrong] = p.border;
            c[ImGuiCol_TableBorderLight] = p.panel;
            c[ImGuiCol_TableRowBg] = colors::transparent;
            c[ImGuiCol_TableRowBgAlt] = with_alpha(p.accent, 0.04f);
            c[ImGuiCol_TextLink] = p.accent;
            c[ImGuiCol_TextSelectedBg] = glow;
            c[ImGuiCol_TreeLines] = p.border;
            c[ImGuiCol_DragDropTarget] = p.accent;
            c[ImGuiCol_DragDropTargetBg] = glow;
            c[ImGuiCol_UnsavedMarker] = p.accent;
            c[ImGuiCol_NavCursor] = p.accent;
            c[ImGuiCol_NavWindowingHighlight] = p.accent;
            c[ImGuiCol_NavWindowingDimBg] = shade;
            c[ImGuiCol_ModalWindowDimBg] = shade;

            return theme;
        }

        // Shared by every theme: larger text and taller table rows than ImGui's defaults.
        void roomy(ImGuiStyle& s) {
            s.FontSizeBase = 22.0f;
            s.CellPadding = {8.0f, 6.0f};
        }

        // Terminal / drafting look: no rounding anywhere, 1px borders on frames, popups and windows.
        void square(ImGuiStyle& s) {
            s.WindowRounding = 0.0f;
            s.ChildRounding = 0.0f;
            s.PopupRounding = 0.0f;
            s.FrameRounding = 0.0f;
            s.ScrollbarRounding = 0.0f;
            s.GrabRounding = 0.0f;
            s.TabRounding = 0.0f;
            s.FrameBorderSize = 1.0f;
            s.PopupBorderSize = 1.0f;
            s.WindowBorderSize = 1.0f;
        }

        // Modern editor look: rounded corners, borderless frames.
        void soft(ImGuiStyle& s) {
            s.WindowRounding = 6.0f;
            s.ChildRounding = 4.0f;
            s.PopupRounding = 4.0f;
            s.FrameRounding = 4.0f;
            s.ScrollbarRounding = 8.0f;
            s.GrabRounding = 4.0f;
            s.TabRounding = 4.0f;
            s.FrameBorderSize = 0.0f;
        }
    }

    Theme Theme::dark() {
        Theme theme;
        ImGui::StyleColorsDark(&theme.style);
        roomy(theme.style);
        return theme;
    }

    Theme Theme::light() {
        Theme theme;
        ImGui::StyleColorsLight(&theme.style);
        roomy(theme.style);
        return theme;
    }

    Theme Theme::classic() {
        Theme theme;
        ImGui::StyleColorsClassic(&theme.style);
        roomy(theme.style);
        return theme;
    }

    Theme Theme::matrix() {
        // Code green (#00FF41) and darker shades of it, on near-black.
        Theme theme = palette({
            .base = from_hex(0x000000FF),
            .bg = from_hex(0x020A04FF),
            .panel = from_hex(0x03140AFF),
            .border = from_hex(0x003B00FF),
            .control = from_hex(0x003B00FF),
            .hover = from_hex(0x008F11FF),
            .active = from_hex(0x00FF41FF),
            .accent = from_hex(0x00FF41FF),
            .text = from_hex(0x00FF41FF),
            .dim = from_hex(0x1F7A3AFF),
        });
        square(theme.style);
        roomy(theme.style);
        return theme;
    }

    Theme Theme::nord() {
        // https://www.nordtheme.com/docs/colors-and-palettes: polar night nord0-3, snow storm nord6,
        // frost nord8 / nord10.
        Theme theme = palette({
            .base = from_hex(0x242933FF),
            .bg = from_hex(0x2E3440FF),
            .panel = from_hex(0x3B4252FF),
            .border = from_hex(0x4C566AFF),
            .control = from_hex(0x434C5EFF),
            .hover = from_hex(0x4C566AFF),
            .active = from_hex(0x5E81ACFF),
            .accent = from_hex(0x88C0D0FF),
            .text = from_hex(0xECEFF4FF),
            .dim = from_hex(0x7B88A1FF),
        });
        soft(theme.style);
        roomy(theme.style);
        return theme;
    }

    Theme Theme::blueprint() {
        // Cyanotype blue paper with white ink.
        Theme theme = palette({
            .base = from_hex(0x133A6BFF),
            .bg = from_hex(0x1B4A85FF),
            .panel = from_hex(0x1F5294FF),
            .border = from_hex(0xBFD7F2FF),
            .control = from_hex(0x245CA3FF),
            .hover = from_hex(0x2E6DBFFF),
            .active = from_hex(0x4A8AD9FF),
            .accent = from_hex(0xFFFFFFFF),
            .text = from_hex(0xF2F7FFFF),
            .dim = from_hex(0x8FB3E0FF),
        });
        square(theme.style);
        roomy(theme.style);
        return theme;
    }

    Theme Theme::one_dark() {
        // Atom One Dark: editor bg, sidebar, gutter, selection, comment grey, blue.
        Theme theme = palette({
            .base = from_hex(0x21252BFF),
            .bg = from_hex(0x282C34FF),
            .panel = from_hex(0x2C313AFF),
            .border = from_hex(0x3E4451FF),
            .control = from_hex(0x3E4451FF),
            .hover = from_hex(0x4B5263FF),
            .active = from_hex(0x528BFFFF),
            .accent = from_hex(0x61AFEFFF),
            .text = from_hex(0xABB2BFFF),
            .dim = from_hex(0x5C6370FF),
        });
        soft(theme.style);
        roomy(theme.style);
        return theme;
    }
}
