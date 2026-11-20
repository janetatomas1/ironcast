#pragma once

#include <memory>
#include <string>

#include <ironcast/widget.hpp>

namespace ironcast::testing {
    // One test: derive from it, build the window's widgets in scene() and check them in body(). Pointers to
    // widgets kept in members by scene() are valid in body(). Running it is the runner's job.
    class Test {
        std::string name_;

    public:
        explicit Test(std::string name);
        virtual ~Test() = default;

        [[nodiscard]] const std::string& name() const;
        // Builds the root widget of the test's window.
        [[nodiscard]] virtual std::unique_ptr<Widget> scene() = 0;
        // Runs the checks once the scene has been laid out.
        virtual void body() = 0;
    };
}
