#pragma once

#include <memory>
#include <string>

#include <ironcast/testing/test.hpp>

// Registers tests and runs them, one test per process: Ironcast::app() can't be reset between tests.
namespace ironcast::testing {
    // Called before main() by IRONCAST_TEST, from static initializers in other translation units.
    void add_test(std::string name, std::unique_ptr<Test> test);

    // The test executable's main(): "--list" prints the test names, "<name>" runs that test headless and
    // returns 0 if it passed.
    int run(int argc, char** argv);
}

#define IRONCAST_TESTING_CAT_(a, b) a##b
#define IRONCAST_TESTING_CAT(a, b) IRONCAST_TESTING_CAT_(a, b)

// IRONCAST_TEST(Fixture, "category", "name") { ...checks... }
//
// Defines and registers the test "category/name"; the braces are its body. Fixture is either a widget, shown
// as the window's root and reachable as scene(), or a Test subclass that sets up the test (window, scene)
// for every test that uses it.
#define IRONCAST_TEST(Fixture, category, name) \
    IRONCAST_TEST_(Fixture, category, name, IRONCAST_TESTING_CAT(IroncastTest_, __LINE__))

// In an anonymous namespace: classes of the same name in other files (same line) must not clash.
#define IRONCAST_TEST_(Fixture, category, name, Class) \
    namespace { \
        class Class final : public ::ironcast::testing::TestBase<Fixture> { \
        public: \
            void body() override; \
        }; \
        [[maybe_unused]] const bool IRONCAST_TESTING_CAT(Class, _registered) = ( \
            ::ironcast::testing::add_test(std::string(category) + "/" + (name), std::make_unique<Class>()), \
            true \
        ); \
    } \
    void Class::body()
