
#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>
#include <algorithm>

#include <SDL3/SDL.h>

#include "ironcast/nullbackend.hpp"
#include "ironcast/testing/testing.hpp"
#include "ironcast/window.hpp"

namespace ironcast {
    namespace testing {
        namespace {
            // Dispatches between the window opening and the body: the root widget is swapped in, then laid out.
            constexpr uint64_t settleFrames = 2;

            bool failed = false;
        }

        // One test per process: Ironcast::app() can't be reset between tests.
        int run(int argc, char** argv) {
            if (argc != 2) {
                std::cerr << std::format("usage: {} --list | <name>\n", argc > 0 ? argv[0] : "tests");
                return 2;
            }

            const std::string_view arg = argv[1];
            if (arg == "--list") {
                for (const auto& test : tests()) {
                    std::cout << test->name() << '\n';
                }
                return 0;
            }

            auto test = std::find_if(
                tests().begin(),
                tests().end(),
                [arg](const auto &test) { return test->name() == arg; }
            );

            if (test != tests().end()) {
                try {
                    test->get()->run();
                } catch (...) {

                }
            }
            std::cerr << std::format("no test named {} (see --list)\n", arg);
            return 1;
        }
    }
}
