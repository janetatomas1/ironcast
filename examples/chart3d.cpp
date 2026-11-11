#include <cmath>
#include <string>

#include <ironcast/ironcast.hpp>
#include <ironcast/opengl/backend.hpp>
#include <ironcast/extras/chart3dwidget.hpp>

using namespace ironcast;

// Chart3D: a helix traced every frame, keeping the last 10 seconds in a Chart3DWidget
// that fills the window. Rings keep the last capacity samples, enough for 10 s at up to 240 fps.

struct Chart3D: Widget {
    static constexpr double window = 10.0;
    static constexpr std::size_t capacity = 2400;

    Chart3DWidget chart = Chart3DWidget("Helix", this);
    Line3DSeries& helix = chart.add_series<Line3DSeries>(
        "helix", SeriesRing(capacity), SeriesRing(capacity), SeriesRing(capacity)
    );

    Chart3D(const std::string& title): Widget(title) {
        column();
        padding(10);
        chart.flex_grow(1);
        chart.set_axis(ImAxis3D_X, {.label = "cos(t)", .limits = ImPlot3DRange(-1.2, 1.2)});
        chart.set_axis(ImAxis3D_Y, {.label = "sin(t)", .limits = ImPlot3DRange(-1.2, 1.2)});
        chart.set_axis(ImAxis3D_Z, {.label = "t"});
        helix.set_line_weight(2.0f);
    }

    bool show() override {
        const double t = ImGui::GetTime();
        helix.xs<SeriesRing>().add(std::cos(t));
        helix.ys<SeriesRing>().add(std::sin(t));
        helix.zs<SeriesRing>().add(t);
        chart.axis(ImAxis3D_Z).limits = ImPlot3DRange(t - window, t);
        return Widget::show();
    }
};

int main(const int argc, char** argv) {
    auto& app = Ironcast::app(argc, argv);
    app.set_backend_factory([](const WindowConfig& config) {
        return std::make_unique<opengl::Backend>(config);
    });
    app.add_window<Chart3D>(std::string("Chart3D"));
    return app.exec();
}
