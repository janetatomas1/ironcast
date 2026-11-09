#pragma once

#include <array>
#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <implot.h>

#include "ironcast/extras/charttheme.hpp"
#include "ironcast/extras/series.hpp"
#include "ironcast/widget.hpp"

namespace ironcast {
    // One plot axis, applied in ImPlot's Setup phase every frame.
    struct Axis {
        // Empty = no label.
        std::string label;
        ImPlotAxisFlags flags = ImPlotAxisFlags_None;
        // Fixed limits; empty = auto-fit to the data.
        std::optional<ImPlotRange> limits;
        ImPlotScale scale = ImPlotScale_Linear;
        // printf format for tick labels, e.g. "%.1f s"; empty = ImPlot's default.
        std::string format;
    };

    // The plot legend, applied in ImPlot's Setup phase every frame.
    struct Legend {
        ImPlotLocation location = ImPlotLocation_NorthWest;
        ImPlotLegendFlags flags = ImPlotLegendFlags_None;
    };

    // An ImPlot plot drawn at this widget's Yoga box. It owns its Series; whether a series
    // owns its values is up to its SeriesData (SeriesBuffer or SeriesView). Intrinsic size is ImPlot's PlotDefaultSize.
    // The title is the plot title; "##" hides it.
    class ChartWidget : public Widget {
        std::vector<std::unique_ptr<Series>> series_;
        // Indexed by ImAxis; empty = disabled. X1 and Y1 are always enabled.
        std::array<std::optional<Axis>, ImAxis_COUNT> axes_ = {Axis{}, {}, {}, Axis{}, {}, {}};
        std::optional<Legend> legend_ = Legend{};
        ImPlotFlags flags_ = ImPlotFlags_None;

    public:
        ChartWidget(const std::string& title = "", Widget* parent = nullptr);

        // The reference stays valid until the series is removed or the chart is destroyed.
        template <std::derived_from<Series> S, class... Args>
        S& add_series(Args&&... args) {
            series_.push_back(std::make_unique<S>(std::forward<Args>(args)...));
            return static_cast<S&>(*series_.back());
        }
        Series& add_series(std::unique_ptr<Series> series);
        // Destroys the series; does nothing when it isn't in this chart.
        void remove_series(const Series& series);
        // Destroys every series with this label.
        void remove_series(const std::string& label);
        // Destroys the series with this id; does nothing when it isn't in this chart.
        void remove_series(uint64_t id);
        // The first series with this label; nullptr when there is none.
        [[nodiscard]] Series* get_series(const std::string& label);
        // The series with this id; nullptr when it isn't in this chart.
        [[nodiscard]] Series* get_series(uint64_t id);
        void clear_series();
        [[nodiscard]] const std::vector<std::unique_ptr<Series>>& series() const;

        // nullptr when the axis is disabled.
        [[nodiscard]] Axis* axis(ImAxis id);
        [[nodiscard]] const Axis* axis(ImAxis id) const;
        // Empty disables the axis; X1 and Y1 can't be disabled.
        ChartWidget& set_axis(ImAxis id, std::optional<Axis> axis);

        // X1 and Y1.
        [[nodiscard]] Axis& x_axis();
        [[nodiscard]] const Axis& x_axis() const;
        ChartWidget& set_x_axis(Axis axis);
        [[nodiscard]] Axis& y_axis();
        [[nodiscard]] const Axis& y_axis() const;
        ChartWidget& set_y_axis(Axis axis);

        // Shown by default; empty = no legend.
        [[nodiscard]] std::optional<Legend>& legend();
        [[nodiscard]] const std::optional<Legend>& legend() const;
        ChartWidget& set_legend(std::optional<Legend> legend);

        [[nodiscard]] ImPlotFlags flags() const;
        ChartWidget& set_flags(ImPlotFlags flags);

        bool show() final;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) final;
    };
}
