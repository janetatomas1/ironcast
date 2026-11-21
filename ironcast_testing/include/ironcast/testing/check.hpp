#pragma once

#include <cmath>
#include <format>
#include <source_location>
#include <string>
#include <string_view>

// Checks for test bodies. CHECK* record a failure and carry on; REQUIRE* record it and end the body. A test
// with any failure exits with 1. Without the IRONCAST_ prefix too, unless that name is already taken (e.g.
// by another test framework).
namespace ironcast::testing {
    // Records a failure at location and prints "file:line: message". What the macros call; usable directly.
    void fail(std::string_view message, std::source_location location = std::source_location::current());

    // Thrown by REQUIRE* to end the body; the runner catches it. Not a std::exception, so a catch in the test
    // doesn't swallow it by accident.
    struct RequireFailed {};

    namespace detail {
        template<typename T>
        std::string describe(const T& value) {
            if constexpr (std::formattable<T, char>) {
                return std::format("{}", value);
            } else {
                return "?";
            }
        }

        template<typename A, typename B>
        bool check_eq(const A& a, const B& b, std::string_view expression, std::source_location location) {
            if (a == b) {
                return true;
            }
            fail(std::format("check failed: {} ({} vs {})", expression, describe(a), describe(b)), location);
            return false;
        }

        template<typename A, typename B, typename E>
        bool check_near(const A& a, const B& b, const E& epsilon, std::string_view expression, std::source_location location) {
            if (std::abs(a - b) <= epsilon) {
                return true;
            }
            fail(std::format(
                "check failed: {} ({} vs {}, more than {} apart)", expression, describe(a), describe(b), describe(epsilon)
            ), location);
            return false;
        }
    }
}

// Records a failure with message, e.g. from a branch the test should never reach. The body carries on.
#define IRONCAST_FAIL(message) ::ironcast::testing::fail(message)

// Records "check failed: cond" when cond is false. The body carries on, so later checks still run.
#define IRONCAST_CHECK(cond) \
    do { \
        if (!(cond)) ::ironcast::testing::fail("check failed: " #cond); \
    } while (0)

// Records "require failed: cond" when cond is false and ends the body, for conditions the rest of the body
// depends on (e.g. a pointer that later checks dereference).
#define IRONCAST_REQUIRE(cond) \
    do { \
        if (!(cond)) { \
            ::ironcast::testing::fail("require failed: " #cond); \
            throw ::ironcast::testing::RequireFailed{}; \
        } \
    } while (0)

// Records a failure when a != b, with both values ("?" for types std::format can't print). The body carries
// on. Evaluates to true if they are equal.
#define IRONCAST_CHECK_EQ(a, b) \
    ::ironcast::testing::detail::check_eq((a), (b), #a " == " #b, std::source_location::current())

// CHECK_EQ, then ends the body if a != b.
#define IRONCAST_REQUIRE_EQ(a, b) \
    do { \
        if (!IRONCAST_CHECK_EQ(a, b)) throw ::ironcast::testing::RequireFailed{}; \
    } while (0)

// Records a failure when a and b are more than epsilon apart (|a - b| > epsilon), with both values, for
// layout sizes and other floats that exact comparison would make flaky. The body carries on. Evaluates to
// true if they are close enough.
#define IRONCAST_CHECK_NEAR(a, b, epsilon) \
    ::ironcast::testing::detail::check_near((a), (b), (epsilon), #a " ~= " #b, std::source_location::current())

// Short names for the macros above, each defined only if nothing else (e.g. another test framework) has
// taken it already.
#ifndef FAIL
#define FAIL(message) IRONCAST_FAIL(message)
#endif
#ifndef CHECK
#define CHECK(cond) IRONCAST_CHECK(cond)
#endif
#ifndef REQUIRE
#define REQUIRE(cond) IRONCAST_REQUIRE(cond)
#endif
#ifndef CHECK_EQ
#define CHECK_EQ(a, b) IRONCAST_CHECK_EQ(a, b)
#endif
#ifndef REQUIRE_EQ
#define REQUIRE_EQ(a, b) IRONCAST_REQUIRE_EQ(a, b)
#endif
#ifndef CHECK_NEAR
#define CHECK_NEAR(a, b, epsilon) IRONCAST_CHECK_NEAR(a, b, epsilon)
#endif
