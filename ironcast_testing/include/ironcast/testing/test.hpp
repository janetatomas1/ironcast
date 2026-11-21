#pragma once

#include <concepts>
#include <memory>
#include <string>

#include <ironcast/widget.hpp>
#include <ironcast/windowconfig.hpp>

namespace ironcast::testing {
    // One test. Usually written with IRONCAST_TEST; derive from it directly (as a fixture) to change the
    // window or share setup between tests. Running it is the runner's job.
    class Test {
        std::string name_;

        // Sets the name, so fixtures need no constructor.
        friend void add_test(std::string name, std::unique_ptr<Test> test);

    public:
        virtual ~Test() = default;

        [[nodiscard]] const std::string& name() const;
        // The test's window. Default: 1280x720, not maximized, titled with the test's name.
        [[nodiscard]] virtual WindowConfig config() const;
        // Builds the root widget of the test's window.
        [[nodiscard]] virtual std::unique_ptr<Widget> make_scene() = 0;
        // Runs the checks once the scene has been laid out.
        virtual void body() = 0;
    };

    // A test whose window shows a Scene, reachable from the body as scene().
    template<std::derived_from<Widget> Scene>
    class SceneTest : public Test {
        Scene* scene_ = nullptr;

    public:
        std::unique_ptr<Widget> make_scene() override {
            auto scene = std::make_unique<Scene>();
            scene_ = scene.get();
            return scene;
        }

        // Valid in body(); the window owns it.
        Scene& scene() {
            return *scene_;
        }
    };

    // What IRONCAST_TEST derives from: a widget becomes SceneTest<widget>, a Test subclass (fixture) is used
    // as it is.
    template<typename Fixture>
    struct FixtureBase {
        static_assert(
            std::derived_from<Fixture, Test>,
            "IRONCAST_TEST: the fixture must derive from either Widget or ironcast::testing::Test"
        );
        using type = Fixture;
    };

    template<std::derived_from<Widget> Fixture>
    struct FixtureBase<Fixture> {
        using type = SceneTest<Fixture>;
    };

    template<typename Fixture>
    using TestBase = typename FixtureBase<Fixture>::type;
}
