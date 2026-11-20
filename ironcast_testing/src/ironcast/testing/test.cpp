#include "ironcast/testing/test.hpp"

namespace ironcast {
    namespace testing {
        namespace {
            // Function-local: tests register from static initializers in other translation units, which may
            // run before a namespace-scope vector in this file has been constructed.
            std::vector<std::unique_ptr<Test>>& registry() {
                static std::vector<std::unique_ptr<Test>> tests;
                return tests;
            }
        }

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

        void add_test(std::unique_ptr<Test> test) {
            registry().push_back(std::move(test));
        }

        const std::vector<std::unique_ptr<Test>>& tests() {
            return registry();
        }
    }
}
