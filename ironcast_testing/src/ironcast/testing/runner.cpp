#include <algorithm>
#include <exception>
#include <format>
#include <iostream>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>

#include "ironcast/ironcast.hpp"
#include "ironcast/nullbackend.hpp"
#include "ironcast/window.hpp"
#include "ironcast/testing/check.hpp"
#include "ironcast/testing/runner.hpp"

namespace ironcast::testing {
    namespace {
        // Dispatches between the window opening and the body: the root widget is swapped in, then laid out.
        constexpr uint64_t settleFrames = 2;

        // Failed checks of the running test; any makes it fail.
        int failures = 0;

        // Function-local: tests register from static initializers in other translation units, which may
        // run before a namespace-scope vector in this file has been constructed.
        std::vector<std::unique_ptr<Test>>& registry() {
            static std::vector<std::unique_ptr<Test>> tests;
            return tests;
        }

        const std::vector<std::unique_ptr<Test>>& tests() {
            return registry();
        }

        // Re-posts itself until the window exists, then runs the body once the scene has settled.
        void start(Ironcast& app, uint64_t windowId, Test& test) {
            if (app.get_window(windowId) == nullptr) {
                app.add_action([&app, windowId, &test] { start(app, windowId, test); }, 1);
                return;
            }
            app.add_action([&app, &test] {
                try {
                    test.body();
                } catch (const RequireFailed&) {
                    // Already recorded by REQUIRE.
                } catch (const std::exception& e) {
                    fail(std::format("uncaught exception: {}", e.what()));
                } catch (...) {
                    fail("uncaught exception");
                }
                app.exit();
            }, settleFrames);
        }

        int run_test(int argc, char** argv, Test& test) {
            // Before Ironcast::app() initializes SDL: windows only in memory, no display needed.
            SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

            auto& app = Ironcast::app(argc, argv);
            app.set_backend_factory([](const WindowConfig& config) {
                return std::make_unique<NullBackend>(config);
            });

            auto window = std::make_unique<Window>(app.make_backend(test.config()));
            Window* raw = window.get();
            std::unique_ptr<Widget> scene = test.make_scene();
            const uint64_t id = app.add_window(std::move(window));
            raw->set_root_widget(std::move(scene));

            app.add_action([&app, id, &test] { start(app, id, test); });
            app.exec();

            if (failures > 0) {
                std::cout << std::format("FAILED {} ({} failed checks)\n", test.name(), failures);
                return 1;
            }
            std::cout << std::format("PASSED {}\n", test.name());
            return 0;
        }
    }

    void fail(std::string_view message, std::source_location location) {
        failures += 1;
        std::cerr << std::format("{}:{}: {}\n", location.file_name(), location.line(), message);
    }

    void add_test(std::string name, std::unique_ptr<Test> test) {
        test->name_ = std::move(name);
        registry().push_back(std::move(test));
    }

    int run(int argc, char** argv) {
        if (argc != 2) {
            std::cerr << std::format("usage: {} --list | <name>\n", argc > 0 ? argv[0] : "tests");
            return 2;
        }

        const std::string_view arg = argv[1];
        const auto& all = tests();
        if (arg == "--list") {
            for (const auto& test : all) {
                std::cout << test->name() << '\n';
            }
            return 0;
        }

        const auto it = std::find_if(
            all.begin(),
            all.end(),
            [arg](const auto& test) {
                return test->name() == arg;
            }
        );
        if (it == all.end()) {
            std::cerr << std::format("no test named {} (see --list)\n", arg);
            return 1;
        }
        return run_test(argc, argv, **it);
    }
}
