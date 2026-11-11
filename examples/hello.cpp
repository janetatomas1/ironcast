#include <ironcast/ironcast.hpp>
#include <ironcast/button.hpp>
#include <ironcast/vulkan/backend.hpp>
#include <ironcast/log.hpp>

using namespace ironcast;
using namespace literals;

struct Hello: Widget {
    Button button = {"Click me", [] { logger().info("clicked"); }, this};

    Hello(const std::string& title): Widget(title) {
        // Percentage margins are of the parent's width, even on the top edge: let the layout centre it.
        center();
        button
        .width(20_pcts)
        .height(10_pcts);
    }
};

int main(int argc, char** argv) {
    auto& app = Ironcast::app(argc, argv);
    app.set_backend_factory([](const WindowConfig& config) {
        return std::make_unique<vulkan::Backend>(config);
    });
    app.add_window<Hello>(std::string("Hello"));
    return app.exec();
}
