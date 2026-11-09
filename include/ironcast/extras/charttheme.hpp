#pragma once

#include <vector>

#include <imgui.h>
#include <implot.h>
#include <implot3d.h>

namespace ironcast {
    class Ironcast;

    // App-wide look of ChartWidget and Chart3DWidget, applied to each window's ImPlot / ImPlot3D context the
    // next time it draws a chart. Plot colours default to auto, which follows the ImGui theme.
    struct ChartTheme {
        ImPlotStyle plot;
        ImPlot3DStyle plot3d;
        // Series colours, registered as a colormap in each context and replacing plot.Colormap and
        // plot3d.Colormap; empty = the styles' own colormaps. ImPlot needs at least two.
        std::vector<ImVec4> palette;
    };

    // Like Ironcast::set_theme(): applied between frames through an action, re-measures every window and
    // redraws. Can be called before any window exists.
    void set_chart_theme(Ironcast& app, const ChartTheme& theme);
    [[nodiscard]] const ChartTheme& chart_theme();

    namespace detail {
        // Makes the current window's ImPlot / ImPlot3D contexts current, creating them on first use and
        // applying the chart theme when it changed. They are destroyed with the window's ImGui context.
        void make_chart_contexts_current();
    }
}
