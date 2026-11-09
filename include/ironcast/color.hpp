#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include <imgui.h>

namespace ironcast {
    // 0xRRGGBBAA -> ImVec4, e.g. from_hex(0xFF8800FF).
    constexpr ImVec4 from_hex(std::uint32_t rgba) {
        return {
            static_cast<float>((rgba >> 24) & 0xFF) / 255.0f,
            static_cast<float>((rgba >> 16) & 0xFF) / 255.0f,
            static_cast<float>((rgba >> 8) & 0xFF) / 255.0f,
            static_cast<float>(rgba & 0xFF) / 255.0f,
        };
    }

    // CSS named colours (https://www.w3.org/TR/css-color-4/#named-colors). Note CSS green is
    // 0x008000FF; pure green is lime.
    namespace colors {
        inline constexpr ImVec4 transparent          = from_hex(0x00000000);
        inline constexpr ImVec4 aliceblue            = from_hex(0xF0F8FFFF);
        inline constexpr ImVec4 antiquewhite         = from_hex(0xFAEBD7FF);
        inline constexpr ImVec4 aqua                 = from_hex(0x00FFFFFF);
        inline constexpr ImVec4 aquamarine           = from_hex(0x7FFFD4FF);
        inline constexpr ImVec4 azure                = from_hex(0xF0FFFFFF);
        inline constexpr ImVec4 beige                = from_hex(0xF5F5DCFF);
        inline constexpr ImVec4 bisque               = from_hex(0xFFE4C4FF);
        inline constexpr ImVec4 black                = from_hex(0x000000FF);
        inline constexpr ImVec4 blanchedalmond       = from_hex(0xFFEBCDFF);
        inline constexpr ImVec4 blue                 = from_hex(0x0000FFFF);
        inline constexpr ImVec4 blueviolet           = from_hex(0x8A2BE2FF);
        inline constexpr ImVec4 brown                = from_hex(0xA52A2AFF);
        inline constexpr ImVec4 burlywood            = from_hex(0xDEB887FF);
        inline constexpr ImVec4 cadetblue            = from_hex(0x5F9EA0FF);
        inline constexpr ImVec4 chartreuse           = from_hex(0x7FFF00FF);
        inline constexpr ImVec4 chocolate            = from_hex(0xD2691EFF);
        inline constexpr ImVec4 coral                = from_hex(0xFF7F50FF);
        inline constexpr ImVec4 cornflowerblue       = from_hex(0x6495EDFF);
        inline constexpr ImVec4 cornsilk             = from_hex(0xFFF8DCFF);
        inline constexpr ImVec4 crimson              = from_hex(0xDC143CFF);
        inline constexpr ImVec4 cyan                 = from_hex(0x00FFFFFF);
        inline constexpr ImVec4 darkblue             = from_hex(0x00008BFF);
        inline constexpr ImVec4 darkcyan             = from_hex(0x008B8BFF);
        inline constexpr ImVec4 darkgoldenrod        = from_hex(0xB8860BFF);
        inline constexpr ImVec4 darkgray             = from_hex(0xA9A9A9FF);
        inline constexpr ImVec4 darkgreen            = from_hex(0x006400FF);
        inline constexpr ImVec4 darkgrey             = from_hex(0xA9A9A9FF);
        inline constexpr ImVec4 darkkhaki            = from_hex(0xBDB76BFF);
        inline constexpr ImVec4 darkmagenta          = from_hex(0x8B008BFF);
        inline constexpr ImVec4 darkolivegreen       = from_hex(0x556B2FFF);
        inline constexpr ImVec4 darkorange           = from_hex(0xFF8C00FF);
        inline constexpr ImVec4 darkorchid           = from_hex(0x9932CCFF);
        inline constexpr ImVec4 darkred              = from_hex(0x8B0000FF);
        inline constexpr ImVec4 darksalmon           = from_hex(0xE9967AFF);
        inline constexpr ImVec4 darkseagreen         = from_hex(0x8FBC8FFF);
        inline constexpr ImVec4 darkslateblue        = from_hex(0x483D8BFF);
        inline constexpr ImVec4 darkslategray        = from_hex(0x2F4F4FFF);
        inline constexpr ImVec4 darkslategrey        = from_hex(0x2F4F4FFF);
        inline constexpr ImVec4 darkturquoise        = from_hex(0x00CED1FF);
        inline constexpr ImVec4 darkviolet           = from_hex(0x9400D3FF);
        inline constexpr ImVec4 deeppink             = from_hex(0xFF1493FF);
        inline constexpr ImVec4 deepskyblue          = from_hex(0x00BFFFFF);
        inline constexpr ImVec4 dimgray              = from_hex(0x696969FF);
        inline constexpr ImVec4 dimgrey              = from_hex(0x696969FF);
        inline constexpr ImVec4 dodgerblue           = from_hex(0x1E90FFFF);
        inline constexpr ImVec4 firebrick            = from_hex(0xB22222FF);
        inline constexpr ImVec4 floralwhite          = from_hex(0xFFFAF0FF);
        inline constexpr ImVec4 forestgreen          = from_hex(0x228B22FF);
        inline constexpr ImVec4 fuchsia              = from_hex(0xFF00FFFF);
        inline constexpr ImVec4 gainsboro            = from_hex(0xDCDCDCFF);
        inline constexpr ImVec4 ghostwhite           = from_hex(0xF8F8FFFF);
        inline constexpr ImVec4 gold                 = from_hex(0xFFD700FF);
        inline constexpr ImVec4 goldenrod            = from_hex(0xDAA520FF);
        inline constexpr ImVec4 gray                 = from_hex(0x808080FF);
        inline constexpr ImVec4 green                = from_hex(0x008000FF);
        inline constexpr ImVec4 greenyellow          = from_hex(0xADFF2FFF);
        inline constexpr ImVec4 grey                 = from_hex(0x808080FF);
        inline constexpr ImVec4 honeydew             = from_hex(0xF0FFF0FF);
        inline constexpr ImVec4 hotpink              = from_hex(0xFF69B4FF);
        inline constexpr ImVec4 indianred            = from_hex(0xCD5C5CFF);
        inline constexpr ImVec4 indigo               = from_hex(0x4B0082FF);
        inline constexpr ImVec4 ivory                = from_hex(0xFFFFF0FF);
        inline constexpr ImVec4 khaki                = from_hex(0xF0E68CFF);
        inline constexpr ImVec4 lavender             = from_hex(0xE6E6FAFF);
        inline constexpr ImVec4 lavenderblush        = from_hex(0xFFF0F5FF);
        inline constexpr ImVec4 lawngreen            = from_hex(0x7CFC00FF);
        inline constexpr ImVec4 lemonchiffon         = from_hex(0xFFFACDFF);
        inline constexpr ImVec4 lightblue            = from_hex(0xADD8E6FF);
        inline constexpr ImVec4 lightcoral           = from_hex(0xF08080FF);
        inline constexpr ImVec4 lightcyan            = from_hex(0xE0FFFFFF);
        inline constexpr ImVec4 lightgoldenrodyellow = from_hex(0xFAFAD2FF);
        inline constexpr ImVec4 lightgray            = from_hex(0xD3D3D3FF);
        inline constexpr ImVec4 lightgreen           = from_hex(0x90EE90FF);
        inline constexpr ImVec4 lightgrey            = from_hex(0xD3D3D3FF);
        inline constexpr ImVec4 lightpink            = from_hex(0xFFB6C1FF);
        inline constexpr ImVec4 lightsalmon          = from_hex(0xFFA07AFF);
        inline constexpr ImVec4 lightseagreen        = from_hex(0x20B2AAFF);
        inline constexpr ImVec4 lightskyblue         = from_hex(0x87CEFAFF);
        inline constexpr ImVec4 lightslategray       = from_hex(0x778899FF);
        inline constexpr ImVec4 lightslategrey       = from_hex(0x778899FF);
        inline constexpr ImVec4 lightsteelblue       = from_hex(0xB0C4DEFF);
        inline constexpr ImVec4 lightyellow          = from_hex(0xFFFFE0FF);
        inline constexpr ImVec4 lime                 = from_hex(0x00FF00FF);
        inline constexpr ImVec4 limegreen            = from_hex(0x32CD32FF);
        inline constexpr ImVec4 linen                = from_hex(0xFAF0E6FF);
        inline constexpr ImVec4 magenta              = from_hex(0xFF00FFFF);
        inline constexpr ImVec4 maroon               = from_hex(0x800000FF);
        inline constexpr ImVec4 mediumaquamarine     = from_hex(0x66CDAAFF);
        inline constexpr ImVec4 mediumblue           = from_hex(0x0000CDFF);
        inline constexpr ImVec4 mediumorchid         = from_hex(0xBA55D3FF);
        inline constexpr ImVec4 mediumpurple         = from_hex(0x9370DBFF);
        inline constexpr ImVec4 mediumseagreen       = from_hex(0x3CB371FF);
        inline constexpr ImVec4 mediumslateblue      = from_hex(0x7B68EEFF);
        inline constexpr ImVec4 mediumspringgreen    = from_hex(0x00FA9AFF);
        inline constexpr ImVec4 mediumturquoise      = from_hex(0x48D1CCFF);
        inline constexpr ImVec4 mediumvioletred      = from_hex(0xC71585FF);
        inline constexpr ImVec4 midnightblue         = from_hex(0x191970FF);
        inline constexpr ImVec4 mintcream            = from_hex(0xF5FFFAFF);
        inline constexpr ImVec4 mistyrose            = from_hex(0xFFE4E1FF);
        inline constexpr ImVec4 moccasin             = from_hex(0xFFE4B5FF);
        inline constexpr ImVec4 navajowhite          = from_hex(0xFFDEADFF);
        inline constexpr ImVec4 navy                 = from_hex(0x000080FF);
        inline constexpr ImVec4 oldlace              = from_hex(0xFDF5E6FF);
        inline constexpr ImVec4 olive                = from_hex(0x808000FF);
        inline constexpr ImVec4 olivedrab            = from_hex(0x6B8E23FF);
        inline constexpr ImVec4 orange               = from_hex(0xFFA500FF);
        inline constexpr ImVec4 orangered            = from_hex(0xFF4500FF);
        inline constexpr ImVec4 orchid               = from_hex(0xDA70D6FF);
        inline constexpr ImVec4 palegoldenrod        = from_hex(0xEEE8AAFF);
        inline constexpr ImVec4 palegreen            = from_hex(0x98FB98FF);
        inline constexpr ImVec4 paleturquoise        = from_hex(0xAFEEEEFF);
        inline constexpr ImVec4 palevioletred        = from_hex(0xDB7093FF);
        inline constexpr ImVec4 papayawhip           = from_hex(0xFFEFD5FF);
        inline constexpr ImVec4 peachpuff            = from_hex(0xFFDAB9FF);
        inline constexpr ImVec4 peru                 = from_hex(0xCD853FFF);
        inline constexpr ImVec4 pink                 = from_hex(0xFFC0CBFF);
        inline constexpr ImVec4 plum                 = from_hex(0xDDA0DDFF);
        inline constexpr ImVec4 powderblue           = from_hex(0xB0E0E6FF);
        inline constexpr ImVec4 purple               = from_hex(0x800080FF);
        inline constexpr ImVec4 rebeccapurple        = from_hex(0x663399FF);
        inline constexpr ImVec4 red                  = from_hex(0xFF0000FF);
        inline constexpr ImVec4 rosybrown            = from_hex(0xBC8F8FFF);
        inline constexpr ImVec4 royalblue            = from_hex(0x4169E1FF);
        inline constexpr ImVec4 saddlebrown          = from_hex(0x8B4513FF);
        inline constexpr ImVec4 salmon               = from_hex(0xFA8072FF);
        inline constexpr ImVec4 sandybrown           = from_hex(0xF4A460FF);
        inline constexpr ImVec4 seagreen             = from_hex(0x2E8B57FF);
        inline constexpr ImVec4 seashell             = from_hex(0xFFF5EEFF);
        inline constexpr ImVec4 sienna               = from_hex(0xA0522DFF);
        inline constexpr ImVec4 silver               = from_hex(0xC0C0C0FF);
        inline constexpr ImVec4 skyblue              = from_hex(0x87CEEBFF);
        inline constexpr ImVec4 slateblue            = from_hex(0x6A5ACDFF);
        inline constexpr ImVec4 slategray            = from_hex(0x708090FF);
        inline constexpr ImVec4 slategrey            = from_hex(0x708090FF);
        inline constexpr ImVec4 snow                 = from_hex(0xFFFAFAFF);
        inline constexpr ImVec4 springgreen          = from_hex(0x00FF7FFF);
        inline constexpr ImVec4 steelblue            = from_hex(0x4682B4FF);
        inline constexpr ImVec4 tan                  = from_hex(0xD2B48CFF);
        inline constexpr ImVec4 teal                 = from_hex(0x008080FF);
        inline constexpr ImVec4 thistle              = from_hex(0xD8BFD8FF);
        inline constexpr ImVec4 tomato               = from_hex(0xFF6347FF);
        inline constexpr ImVec4 turquoise            = from_hex(0x40E0D0FF);
        inline constexpr ImVec4 violet               = from_hex(0xEE82EEFF);
        inline constexpr ImVec4 wheat                = from_hex(0xF5DEB3FF);
        inline constexpr ImVec4 white                = from_hex(0xFFFFFFFF);
        inline constexpr ImVec4 whitesmoke           = from_hex(0xF5F5F5FF);
        inline constexpr ImVec4 yellow               = from_hex(0xFFFF00FF);
        inline constexpr ImVec4 yellowgreen          = from_hex(0x9ACD32FF);
    }

    // Parses "#rgb", "#rgba", "#rrggbb", "#rrggbbaa" (or with "0x" instead of '#'), "rgb[a](r, g, b[, a])",
    // "hsv(h, s, v[, a])" or "hsl[a](h, s, l[, a])". r, g, b 0-255; h 0-360; s, v, l 0-100; a 0-1.
    // std::nullopt if malformed or out of range.
    [[nodiscard]] std::optional<ImVec4> parse_color(std::string_view s);

    namespace literals {
        // "#ff8800"_rgb, "255, 136, 0"_rgb, "255, 136, 0, 0.5"_rgb. Same formats and ranges as parse_color.
        // Invalid input asserts in debug and gives transparent black in release.
        ImVec4 operator""_rgb(const char* s, std::size_t n);

        // "32, 100, 100"_hsv, "32, 100, 100, 0.5"_hsv. Same formats and ranges as parse_color.
        // Invalid input asserts in debug and gives transparent black in release.
        ImVec4 operator""_hsv(const char* s, std::size_t n);

        // "32, 100, 50"_hsl, "32, 100, 50, 0.5"_hsl. Same formats and ranges as parse_color.
        // Invalid input asserts in debug and gives transparent black in release.
        ImVec4 operator""_hsl(const char* s, std::size_t n);
    }
}
