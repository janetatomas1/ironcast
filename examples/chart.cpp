#include <cmath>
#include <string>

#include <ironcast/ironcast.hpp>
#include <ironcast/vulkan/backend.hpp>
#include <ironcast/extras/chartwidget.hpp>

using namespace ironcast;

// Chart: sine and cosine sampled every frame, scrolling through the last 10 seconds
// in a ChartWidget that fills the window. Rings keep the last capacity samples, enough for
// 10 s at up to 240 fps.

struct Chart: Widget {
    static constexpr double window = 10.0;
    static constexpr std::size_t capacity = 2400;

    ChartWidget chart = ChartWidget("Sine and cosine", this);
    LineSeries& sine = chart.add_series<LineSeries>("sin(t)", SeriesRing(capacity), SeriesRing(capacity));
    LineSeries& cosine = chart.add_series<LineSeries>("cos(t)", SeriesRing(capacity), SeriesRing(capacity));

    Chart(const std::string& title): Widget(title) {
        column();
        padding(10);
        chart.flex_grow(1);
        chart.set_x_axis({.label = "t", .format = "%.0f s"});
        chart.set_y_axis({.label = "y", .limits = ImPlotRange(-1.2, 1.2)});
    }

    static void sample(LineSeries& series, double t, double y) {
        series.xs<SeriesRing>().add(t);
        series.ys<SeriesRing>().add(y);
    }

    bool show() override {
        const double t = ImGui::GetTime();
        sample(sine, t, std::sin(t));
        sample(cosine, t, std::cos(t));
        chart.x_axis().limits = ImPlotRange(t - window, t);
        return Widget::show();
    }
};

int main(const int argc, char** argv) {
    auto& app = Ironcast::app(argc, argv);

    app.set_backend_factory([](const WindowConfig& config) {
        return std::make_unique<vulkan::Backend>(config);
    });
    app.add_window<Chart>(std::string("Chart"));

    return app.exec();
}
