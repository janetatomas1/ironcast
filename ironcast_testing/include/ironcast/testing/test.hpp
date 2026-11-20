#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <ironcast/widget.hpp>

namespace ironcast {
    namespace testing {
        // One test: what it shows and what it checks. Running it is the runner's job.
        class Test {
            std::string name_;
            std::function<std::unique_ptr<Widget>()> scene_;
            std::function<void(Widget&)> body_;

        public:
            Test(
                std::string name,
                std::function<std::unique_ptr<Widget>()> scene,
                std::function<void(Widget&)> body
            );

            [[nodiscard]] const std::string& name() const;
            // Builds the root widget of the test's window.
            [[nodiscard]] std::unique_ptr<Widget> scene() const;
            // Runs the checks on the scene once it has been laid out.
            void body(Widget& scene) const;
        };

        // Registry, filled before main() by tests in other translation units.
        void add_test(std::unique_ptr<Test> test);
        const std::vector<std::unique_ptr<Test>>& tests();
    }
}
