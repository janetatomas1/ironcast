#include <ironcast/button.hpp>
#include <ironcast/testing/testing.hpp>

using namespace ironcast;
using namespace literals;

namespace {
    // A button centred in the window, 20% x 10% of it.
    struct Centred : Widget {
        Button button = {"Click me", this};

        Centred() {
            center();
            button
            .width(20_pcts)
            .height(10_pcts);
        }
    };

    // The same scene in a smaller window.
    struct SmallCentred : testing::SceneTest<Centred> {
        WindowConfig config() const override {
            auto config = SceneTest::config();
            config.width = 400;
            config.height = 300;
            return config;
        }
    };

    // Yoga rounds to whole pixels.
    constexpr float slack = 1.0f;
}

IRONCAST_TEST(Centred, "layout", "button_size") {
    REQUIRE(scene().width() > 0);
    REQUIRE(scene().height() > 0);
    CHECK_NEAR(scene().button.width(), scene().width() * 0.2f, slack);
    CHECK_NEAR(scene().button.height(), scene().height() * 0.1f, slack);
}

IRONCAST_TEST(Centred, "layout", "button_centred") {
    const auto& button = scene().button;
    CHECK_NEAR(button.x() + button.width() / 2, scene().width() / 2, slack);
    CHECK_NEAR(button.y() + button.height() / 2, scene().height() / 2, slack);
}

// Drawn where the layout put it: ImGui's item matches the button's Yoga box.
IRONCAST_TEST(Centred, "layout", "button_drawn") {
    const auto& button = scene().button;
    const ImVec2 min = button.drawn_min();
    const ImVec2 max = button.drawn_max();
    CHECK_NEAR(max.x - min.x, button.width(), slack);
    CHECK_NEAR(max.y - min.y, button.height(), slack);
    // Positions are relative to the parent, the root here.
    CHECK_NEAR(min.x - scene().drawn_min().x, button.x(), slack);
    CHECK_NEAR(min.y - scene().drawn_min().y, button.y(), slack);
}

IRONCAST_TEST(SmallCentred, "layout", "small_window") {
    CHECK(scene().width() <= 400);
    CHECK(scene().height() <= 300);
    CHECK_NEAR(scene().button.width(), scene().width() * 0.2f, slack);
}
