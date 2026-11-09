#include <algorithm>

#include "ironcast/assert.hpp"

#include "ironcast/extras/chartwidget.hpp"

namespace ironcast {
    namespace {
        void setup_axis(ImAxis id, const Axis& axis) {
            ImPlot::SetupAxis(id, axis.label.empty() ? nullptr : axis.label.c_str(), axis.flags);
            ImPlot::SetupAxisScale(id, axis.scale);
            if (axis.limits) {
                ImPlot::SetupAxisLimits(id, axis.limits->Min, axis.limits->Max, ImPlotCond_Always);
            }
            if (!axis.format.empty()) {
                ImPlot::SetupAxisFormat(id, axis.format.c_str());
            }
        }
    }

    ChartWidget::ChartWidget(const std::string& title, Widget* parent): Widget(title, parent) {
        layout().enable_measure();
    }

    Series& ChartWidget::add_series(std::unique_ptr<Series> series) {
        series_.push_back(std::move(series));
        return *series_.back();
    }

    void ChartWidget::remove_series(const Series& series) {
        std::erase_if(series_, [&](const auto& s) { return s.get() == &series; });
    }

    void ChartWidget::remove_series(const std::string& label) {
        std::erase_if(series_, [&](const auto& s) { return s->label() == label; });
    }

    Series* ChartWidget::get_series(const std::string& label) {
        const auto it = std::ranges::find_if(series_, [&](const auto& s) { return s->label() == label; });
        return it == series_.end() ? nullptr : it->get();
    }

    void ChartWidget::remove_series(uint64_t id) {
        std::erase_if(series_, [&](const auto& s) { return s->id() == id; });
    }

    Series* ChartWidget::get_series(uint64_t id) {
        const auto it = std::ranges::find_if(series_, [&](const auto& s) { return s->id() == id; });
        return it == series_.end() ? nullptr : it->get();
    }

    void ChartWidget::clear_series() {
        series_.clear();
    }

    const std::vector<std::unique_ptr<Series>>& ChartWidget::series() const {
        return series_;
    }

    Axis* ChartWidget::axis(ImAxis id) {
        return axes_[id] ? &*axes_[id] : nullptr;
    }

    const Axis* ChartWidget::axis(ImAxis id) const {
        return axes_[id] ? &*axes_[id] : nullptr;
    }

    ChartWidget& ChartWidget::set_axis(ImAxis id, std::optional<Axis> axis) {
        IRONCAST_DEBUG_ASSERT(axis || (id != ImAxis_X1 && id != ImAxis_Y1), "ChartWidget: X1 and Y1 can't be disabled", id);
        axes_[id] = std::move(axis);
        return *this;
    }

    Axis& ChartWidget::x_axis() {
        return *axes_[ImAxis_X1];
    }

    const Axis& ChartWidget::x_axis() const {
        return *axes_[ImAxis_X1];
    }

    ChartWidget& ChartWidget::set_x_axis(Axis axis) {
        return set_axis(ImAxis_X1, std::move(axis));
    }

    Axis& ChartWidget::y_axis() {
        return *axes_[ImAxis_Y1];
    }

    const Axis& ChartWidget::y_axis() const {
        return *axes_[ImAxis_Y1];
    }

    ChartWidget& ChartWidget::set_y_axis(Axis axis) {
        return set_axis(ImAxis_Y1, std::move(axis));
    }

    std::optional<Legend>& ChartWidget::legend() {
        return legend_;
    }

    const std::optional<Legend>& ChartWidget::legend() const {
        return legend_;
    }

    ChartWidget& ChartWidget::set_legend(std::optional<Legend> legend) {
        legend_ = legend;
        return *this;
    }

    ImPlotFlags ChartWidget::flags() const {
        return flags_;
    }

    ChartWidget& ChartWidget::set_flags(ImPlotFlags flags) {
        flags_ = flags;
        return *this;
    }

    bool ChartWidget::show() {
        detail::make_chart_contexts_current();
        // title_id() carries the "##id", so charts with the same title don't share state.
        if (!ImPlot::BeginPlot(title_id().c_str(), size(), legend_ ? flags_ : flags_ | ImPlotFlags_NoLegend)) {
            return false;
        }
        for (int id = 0; id < ImAxis_COUNT; ++id) {
            if (axes_[id]) {
                setup_axis(id, *axes_[id]);
            }
        }
        if (legend_) {
            ImPlot::SetupLegend(legend_->location, legend_->flags);
        }
        for (const auto& series : series_) {
            // A series on a disabled axis is skipped; ImPlot would assert in SetAxes.
            if (!axes_[series->x_axis()] || !axes_[series->y_axis()]) {
                continue;
            }
            ImPlot::SetAxes(series->x_axis(), series->y_axis());
            series->plot();
        }
        ImPlot::EndPlot();
        return false;
    }

    ImVec2 ChartWidget::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        // From the theme: the window's ImPlot context may not exist before its first chart is drawn.
        const ImVec2 intrinsic = chart_theme().plot.PlotDefaultSize;
        return ImVec2{
            fit_measure(intrinsic.x, width, widthMode),
            fit_measure(intrinsic.y, height, heightMode)
        };
    }
}
