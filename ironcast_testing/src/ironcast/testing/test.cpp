#include "ironcast/testing/test.hpp"

namespace ironcast::testing {
    Test::Test(std::string name)
        : name_(std::move(name)) {}

    const std::string& Test::name() const {
        return name_;
    }
}
