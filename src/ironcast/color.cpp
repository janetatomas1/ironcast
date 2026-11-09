#include <algorithm>
#include <array>
#include <charconv>

#include "ironcast/assert.hpp"

#include "ironcast/color.hpp"

namespace ironcast {
    namespace {
        std::string_view trim(std::string_view s) {
            const auto first = s.find_first_not_of(" \t");
            if (first == std::string_view::npos) {
                return {};
            }
            return s.substr(first, s.find_last_not_of(" \t") - first + 1);
        }

        // Number in [0, max]; the whole (trimmed) string must be consumed.
        std::optional<float> parse_number(std::string_view s, float max) {
            s = trim(s);
            float value = 0.0f;
            const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
            // the negated range check also rejects nan
            if (ec != std::errc{} || ptr != s.data() + s.size() || !(value >= 0.0f && value <= max)) {
                return std::nullopt;
            }
            return value;
        }

        // "a, b, c" or "a, b, c, alpha" with a, b, c in [0, max]; alpha is 0-1 and defaults to 1.
        std::optional<std::array<float, 4>> parse_components(std::string_view s, const std::array<float, 3>& max) {
            std::array<float, 4> out{0.0f, 0.0f, 0.0f, 1.0f};
            std::size_t n = 0;
            while (true) {
                if (n == out.size()) {
                    return std::nullopt;
                }
                const auto comma = s.find(',');
                const auto value = parse_number(s.substr(0, comma), n < max.size() ? max[n] : 1.0f);
                if (!value) {
                    return std::nullopt;
                }
                out[n++] = *value;
                if (comma == std::string_view::npos) {
                    break;
                }
                s.remove_prefix(comma + 1);
            }
            if (n < 3) {
                return std::nullopt;
            }
            return out;
        }

        // "rgb", "rgba", "rrggbb" or "rrggbbaa" (no '#').
        std::optional<ImVec4> parse_hex(std::string_view s) {
            const bool shorthand = s.size() == 3 || s.size() == 4;
            if (!shorthand && s.size() != 6 && s.size() != 8) {
                return std::nullopt;
            }
            const std::size_t width = shorthand ? 1 : 2;
            std::array<float, 4> out{0.0f, 0.0f, 0.0f, 1.0f};
            for (std::size_t i = 0; i < s.size() / width; ++i) {
                const char* first = s.data() + i * width;
                unsigned value = 0;
                const auto [ptr, ec] = std::from_chars(first, first + width, value, 16);
                if (ec != std::errc{} || ptr != first + width) {
                    return std::nullopt;
                }
                // #f80 == #ff8800
                out[i] = static_cast<float>(shorthand ? value * 17 : value) / 255.0f;
            }
            return ImVec4(out[0], out[1], out[2], out[3]);
        }

        std::optional<ImVec4> parse_rgb(std::string_view s) {
            const auto c = parse_components(s, {255.0f, 255.0f, 255.0f});
            if (!c) {
                return std::nullopt;
            }
            return ImVec4((*c)[0] / 255.0f, (*c)[1] / 255.0f, (*c)[2] / 255.0f, (*c)[3]);
        }

        std::optional<ImVec4> parse_hsv(std::string_view s) {
            const auto c = parse_components(s, {360.0f, 100.0f, 100.0f});
            if (!c) {
                return std::nullopt;
            }
            ImVec4 out{0.0f, 0.0f, 0.0f, (*c)[3]};
            // ImGui takes hue in [0, 1) and wraps 1 back to red
            ImGui::ColorConvertHSVtoRGB((*c)[0] / 360.0f, (*c)[1] / 100.0f, (*c)[2] / 100.0f, out.x, out.y, out.z);
            return out;
        }

        std::optional<ImVec4> parse_hsl(std::string_view s) {
            const auto c = parse_components(s, {360.0f, 100.0f, 100.0f});
            if (!c) {
                return std::nullopt;
            }
            // HSL -> HSV, then let ImGui do HSV -> RGB
            const float sat = (*c)[1] / 100.0f;
            const float light = (*c)[2] / 100.0f;
            const float val = light + sat * std::min(light, 1.0f - light);
            const float hsv_sat = val > 0.0f ? 2.0f * (1.0f - light / val) : 0.0f;
            ImVec4 out{0.0f, 0.0f, 0.0f, (*c)[3]};
            ImGui::ColorConvertHSVtoRGB((*c)[0] / 360.0f, hsv_sat, val, out.x, out.y, out.z);
            return out;
        }

        std::optional<std::string_view> unwrap(std::string_view s, std::string_view prefix) {
            if (!s.starts_with(prefix) || !s.ends_with(')')) {
                return std::nullopt;
            }
            return s.substr(prefix.size(), s.size() - prefix.size() - 1);
        }
    }

    std::optional<ImVec4> parse_color(std::string_view s) {
        s = trim(s);
        if (s.starts_with('#')) {
            return parse_hex(s.substr(1));
        }
        if (s.starts_with("0x")) {
            return parse_hex(s.substr(2));
        }
        // rgba/hsla are aliases: alpha is optional in both spellings
        for (const auto prefix : {"rgb(", "rgba("}) {
            if (const auto args = unwrap(s, prefix)) {
                return parse_rgb(*args);
            }
        }
        if (const auto args = unwrap(s, "hsv(")) {
            return parse_hsv(*args);
        }
        for (const auto prefix : {"hsl(", "hsla("}) {
            if (const auto args = unwrap(s, prefix)) {
                return parse_hsl(*args);
            }
        }
        return std::nullopt;
    }

    namespace literals {
        ImVec4 operator""_rgb(const char* s, std::size_t n) {
            const auto str = trim({s, n});
            const auto c = str.starts_with('#') ? parse_hex(str.substr(1)) : parse_rgb(str);
            IRONCAST_DEBUG_ASSERT(c.has_value(), "invalid rgb colour literal", str);
            return c.value_or(ImVec4{});
        }

        ImVec4 operator""_hsv(const char* s, std::size_t n) {
            const auto c = parse_hsv({s, n});
            IRONCAST_DEBUG_ASSERT(c.has_value(), "invalid hsv colour literal", std::string_view{s, n});
            return c.value_or(ImVec4{});
        }

        ImVec4 operator""_hsl(const char* s, std::size_t n) {
            const auto c = parse_hsl({s, n});
            IRONCAST_DEBUG_ASSERT(c.has_value(), "invalid hsl colour literal", std::string_view{s, n});
            return c.value_or(ImVec4{});
        }
    }
}
