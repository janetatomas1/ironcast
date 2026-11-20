#pragma once

#include <memory>

#include <ironcast/testing/test.hpp>

// Registers tests and runs them, one test per process: Ironcast::app() can't be reset between tests.
namespace ironcast::testing {
    // Called before main() by tests in other translation units.
    void add_test(std::unique_ptr<Test> test);

    // The test executable's main(): "--list" prints the test names, "<name>" runs that test headless and
    // returns 0 if it passed.
    int run(int argc, char** argv);
}
