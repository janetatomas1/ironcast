// parse_color, the colour literals and from_hex. No ImGui context needed: the HSV conversion is a
// static ImGui helper.

#include <gtest/gtest.h>

#include <optional>
#include <string_view>

#include <ironcast/color.hpp>

using namespace ironcast;
using namespace ironcast::literals;

namespace {
    // One 8-bit step is 1/255; HSV/HSL go through float maths.
    constexpr float tolerance = 1e-4f;

    void expect_color(const ImVec4& actual, const ImVec4& expected) {
        EXPECT_NEAR(actual.x, expected.x, tolerance);
        EXPECT_NEAR(actual.y, expected.y, tolerance);
        EXPECT_NEAR(actual.z, expected.z, tolerance);
        EXPECT_NEAR(actual.w, expected.w, tolerance);
    }

    void expect_parses(std::string_view s, const ImVec4& expected) {
        SCOPED_TRACE(s);
        const std::optional<ImVec4> c = parse_color(s);
        ASSERT_TRUE(c.has_value());
        expect_color(*c, expected);
    }

    void expect_rejects(std::string_view s) {
        EXPECT_FALSE(parse_color(s).has_value()) << s;
    }
}

TEST(FromHex, ChannelOrderIsRgba) {
    expect_color(from_hex(0xFF000000), {1.0f, 0.0f, 0.0f, 0.0f});
    expect_color(from_hex(0x00FF0000), {0.0f, 1.0f, 0.0f, 0.0f});
    expect_color(from_hex(0x0000FF00), {0.0f, 0.0f, 1.0f, 0.0f});
    expect_color(from_hex(0x000000FF), {0.0f, 0.0f, 0.0f, 1.0f});
    expect_color(from_hex(0xFF880080), {1.0f, 136.0f / 255.0f, 0.0f, 128.0f / 255.0f});
}

TEST(NamedColors, CssGreenIsNotLime) {
    expect_color(colors::green, {0.0f, 128.0f / 255.0f, 0.0f, 1.0f});
    expect_color(colors::lime, {0.0f, 1.0f, 0.0f, 1.0f});
    expect_color(colors::rebeccapurple, from_hex(0x663399FF));
}

TEST(ParseColor, HexLong) {
    expect_parses("#ff8800", from_hex(0xFF8800FF));
    expect_parses("#FF8800", from_hex(0xFF8800FF));
    expect_parses("#ff880080", from_hex(0xFF880080));
    expect_parses("0xff8800", from_hex(0xFF8800FF));
    expect_parses("0xff880080", from_hex(0xFF880080));
}

TEST(ParseColor, HexShorthandRepeatsDigits) {
    expect_parses("#f80", from_hex(0xFF8800FF));
    expect_parses("#f808", from_hex(0xFF880088));
    expect_parses("0xf80", from_hex(0xFF8800FF));
}

TEST(ParseColor, HexRejectsBadLengthAndDigits) {
    expect_rejects("#");
    expect_rejects("0x");
    expect_rejects("#ff");
    expect_rejects("#ff880");
    expect_rejects("#ff88000");
    expect_rejects("#ff8800001");
    expect_rejects("#gg8800");
    expect_rejects("#-f8800");
}

TEST(ParseColor, Rgb) {
    expect_parses("rgb(255, 136, 0)", from_hex(0xFF8800FF));
    expect_parses("rgb(255,136,0)", from_hex(0xFF8800FF));
    expect_parses("rgb(255, 136, 0, 0.5)", {1.0f, 136.0f / 255.0f, 0.0f, 0.5f});
    expect_parses("rgba(255, 136, 0, 0.5)", {1.0f, 136.0f / 255.0f, 0.0f, 0.5f});
    // alpha is optional in the rgba spelling too
    expect_parses("rgba(255, 136, 0)", from_hex(0xFF8800FF));
    expect_parses("rgb(127.5, 0, 0)", {0.5f, 0.0f, 0.0f, 1.0f});
}

TEST(ParseColor, RangeEndsAreInclusive) {
    expect_parses("rgb(0, 0, 0, 0)", {0.0f, 0.0f, 0.0f, 0.0f});
    expect_parses("rgb(255, 255, 255, 1)", {1.0f, 1.0f, 1.0f, 1.0f});
    expect_parses("hsv(360, 100, 100)", {1.0f, 0.0f, 0.0f, 1.0f});
}

TEST(ParseColor, TrimsWhitespace) {
    expect_parses("  #ff8800\t", from_hex(0xFF8800FF));
    expect_parses(" rgb( 255 ,\t136 , 0 ) ", from_hex(0xFF8800FF));
}

TEST(ParseColor, RgbRejectsMalformed) {
    expect_rejects("rgb(1, 2)");
    expect_rejects("rgb(1, 2, 3, 0.5, 1)");
    expect_rejects("rgb(1, , 3)");
    expect_rejects("rgb()");
    expect_rejects("rgb(1, 2, 3");
    expect_rejects("rgb(1, 2, 3)x");
    expect_rejects("rgb(1 2 3)");
    expect_rejects("rgb(1, 2, 3px)");
}

TEST(ParseColor, RgbRejectsOutOfRange) {
    expect_rejects("rgb(256, 0, 0)");
    expect_rejects("rgb(-1, 0, 0)");
    expect_rejects("rgb(0, 0, 0, 1.5)");
    expect_rejects("rgb(0, 0, 0, -0.1)");
    expect_rejects("rgb(nan, 0, 0)");
    expect_rejects("rgb(inf, 0, 0)");
}

TEST(ParseColor, Hsv) {
    expect_parses("hsv(0, 100, 100)", {1.0f, 0.0f, 0.0f, 1.0f});
    expect_parses("hsv(120, 100, 100)", {0.0f, 1.0f, 0.0f, 1.0f});
    expect_parses("hsv(240, 100, 100, 0.5)", {0.0f, 0.0f, 1.0f, 0.5f});
    expect_parses("hsv(0, 0, 50)", {0.5f, 0.5f, 0.5f, 1.0f});
}

TEST(ParseColor, Hsl) {
    expect_parses("hsl(0, 100, 50)", {1.0f, 0.0f, 0.0f, 1.0f});
    expect_parses("hsl(120, 100, 25)", {0.0f, 0.5f, 0.0f, 1.0f});
    expect_parses("hsla(240, 100, 50, 0.5)", {0.0f, 0.0f, 1.0f, 0.5f});
    expect_parses("hsl(0, 0, 100)", {1.0f, 1.0f, 1.0f, 1.0f});
    expect_parses("hsl(200, 100, 0)", {0.0f, 0.0f, 0.0f, 1.0f});
    expect_parses("hsl(0, 0, 50)", {0.5f, 0.5f, 0.5f, 1.0f});
}

TEST(ParseColor, HsvHslRejectOutOfRange) {
    expect_rejects("hsv(361, 0, 0)");
    expect_rejects("hsv(0, 101, 0)");
    expect_rejects("hsv(0, 0, 101)");
    expect_rejects("hsl(361, 0, 0)");
    expect_rejects("hsl(0, 101, 0)");
    expect_rejects("hsl(0, 0, 101)");
}

TEST(ParseColor, RejectsUnknownForms) {
    expect_rejects("");
    expect_rejects("   ");
    expect_rejects("red");
    expect_rejects("ff8800");
    expect_rejects("hsva(0, 0, 0)");
    expect_rejects("rgb[1, 2, 3]");
}

TEST(ColorLiterals, Rgb) {
    expect_color("#ff8800"_rgb, from_hex(0xFF8800FF));
    expect_color("255, 136, 0"_rgb, from_hex(0xFF8800FF));
    expect_color("255, 136, 0, 0.5"_rgb, {1.0f, 136.0f / 255.0f, 0.0f, 0.5f});
}

TEST(ColorLiterals, HsvHsl) {
    expect_color("120, 100, 100"_hsv, {0.0f, 1.0f, 0.0f, 1.0f});
    expect_color("120, 100, 100, 0.5"_hsv, {0.0f, 1.0f, 0.0f, 0.5f});
    expect_color("120, 100, 25"_hsl, {0.0f, 0.5f, 0.0f, 1.0f});
    expect_color("120, 100, 25, 0.5"_hsl, {0.0f, 0.5f, 0.0f, 0.5f});
}
