#include "ironcast/testing/test.hpp"

namespace ironcast::testing {
    Test::Test(
        std::string name,
        std::function<std::unique_ptr<Widget>()> scene,
        std::function<void(Widget&)> body
    )
        : name_(std::move(name)), scene_(std::move(scene)), body_(std::move(body)) {}

    const std::string& Test::name() const {
        return name_;
    }

    std::unique_ptr<Widget> Test::scene() const {
        return scene_();
    }

    void Test::body(Widget& scene) const {
        body_(scene);
    }
}
