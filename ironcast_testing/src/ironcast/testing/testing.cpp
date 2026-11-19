
#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>

#include "ironcast/nullbackend.hpp"
#include "ironcast/testing/testing.hpp"
#include "ironcast/window.hpp"

namespace ironcast {
    namespace testing {
        namespace {
            std::vector<std::unique_ptr<Test>> tests_;
        };

        void add_test(std::unique_ptr<Test> test) {
            tests_.push_back(std::move(test));
        }

        const std::vector<std::unique_ptr<Test>>& tests() {
            return tests_;
        }

        namespace {
            // Dispatches between the window opening and the body: the root widget is swapped in, then laid out.
            constexpr uint64_t settleFrames = 2;

            bool failed = false;

            // Re-posts itself until the window exists, then runs the body after the scene has settled.
            void start(Ironcast& app, uint64_t windowId, Widget& scene, const Test& test) {
                if (app.get_window(windowId) == nullptr) {
                    app.add_action([&app, windowId, &scene, &test] { start(app, windowId, scene, test); }, 1);
                    return;
                }
                app.add_action([&app, &scene, &test] {
                    try {
                        test.body(scene);
                    } catch (const std::exception& e) {
                        std::cerr << std::format("uncaught exception: {}\n", e.what());
                        failed = true;
                    } catch (...) {
                        std::cerr << "uncaught exception\n";
                        failed = true;
                    }
                    app.exit();
                }, settleFrames);
            }

            int run_test(int argc, char** argv, const Test& test) {
                auto& app = Ironcast::app(argc, argv);
                app.set_backend_factory([](const WindowConfig& config) {
                    return std::make_unique<NullBackend>(config);
                });

                // Fixed size, not maximized, so results don't depend on the machine.
                WindowConfig config;
                config.width = 1280;
                config.height = 720;
                config.title = test.name;
                config.windowState = WindowState::NORMAL;

                auto window = std::make_unique<Window>(app.make_backend(config));
                Window* raw = window.get();
                std::unique_ptr<Widget> scene = test.scene();
                Widget& sceneRef = *scene; // before the move
                const uint64_t id = app.add_window(std::move(window));
                raw->set_root_widget(std::move(scene));

                app.add_action([&app, id, &sceneRef, &test] { start(app, id, sceneRef, test); });
                app.exec();

                std::cout << std::format("{} {}\n", failed ? "FAILED" : "PASSED", test.name);
                return failed ? 1 : 0;
            }
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
                    std::cout << test->name << '\n';
                }
                return 0;
            }

            for (const auto& test : tests()) {
                if (test->name == arg) {
                    return run_test(argc, argv, *test);
                }
            }
            std::cerr << std::format("no test named {} (see --list)\n", arg);
            return 1;
        }
    }
}
