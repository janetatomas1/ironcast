#include <algorithm>

#include "ironcast/extras/chart3dwidget.hpp"

namespace ironcast {
    namespace {
        void setup_axis(ImAxis3D id, const Axis3D& axis) {
            ImPlot3D::SetupAxis(id, axis.label.empty() ? nullptr : axis.label.c_str(), axis.flags);
            ImPlot3D::SetupAxisScale(id, axis.scale);
            if (axis.limits) {
                ImPlot3D::SetupAxisLimits(id, axis.limits->Min, axis.limits->Max, ImPlot3DCond_Always);
            }
        }
    }

    Chart3DWidget::Chart3DWidget(const std::string& title, Widget* parent): Widget(title, parent) {
        layout().enable_measure();
    }

    Series3D& Chart3DWidget::add_series(std::unique_ptr<Series3D> series) {
        series_.push_back(std::move(series));
        return *series_.back();
    }

    void Chart3DWidget::remove_series(const Series3D& series) {
        std::erase_if(series_, [&](const auto& s) { return s.get() == &series; });
    }

    void Chart3DWidget::remove_series(const std::string& label) {
        std::erase_if(series_, [&](const auto& s) { return s->label() == label; });
    }

    void Chart3DWidget::remove_series(uint64_t id) {
        std::erase_if(series_, [&](const auto& s) { return s->id() == id; });
    }

    Series3D* Chart3DWidget::get_series(const std::string& label) {
        const auto it = std::ranges::find_if(series_, [&](const auto& s) { return s->label() == label; });
        return it == series_.end() ? nullptr : it->get();
    }

    Series3D* Chart3DWidget::get_series(uint64_t id) {
        const auto it = std::ranges::find_if(series_, [&](const auto& s) { return s->id() == id; });
        return it == series_.end() ? nullptr : it->get();
    }

    void Chart3DWidget::clear_series() {
        series_.clear();
    }

    const std::vector<std::unique_ptr<Series3D>>& Chart3DWidget::series() const {
        return series_;
    }

    Axis3D& Chart3DWidget::axis(ImAxis3D id) {
        return axes_[id];
    }

    const Axis3D& Chart3DWidget::axis(ImAxis3D id) const {
        return axes_[id];
    }

    Chart3DWidget& Chart3DWidget::set_axis(ImAxis3D id, Axis3D axis) {
        axes_[id] = std::move(axis);
        return *this;
    }

    std::optional<Legend3D>& Chart3DWidget::legend() {
        return legend_;
    }

    const std::optional<Legend3D>& Chart3DWidget::legend() const {
        return legend_;
    }

    Chart3DWidget& Chart3DWidget::set_legend(std::optional<Legend3D> legend) {
        legend_ = legend;
        return *this;
    }

    ImPlot3DFlags Chart3DWidget::flags() const {
        return flags_;
    }

    Chart3DWidget& Chart3DWidget::set_flags(ImPlot3DFlags flags) {
        flags_ = flags;
        return *this;
    }

    bool Chart3DWidget::show() {
        detail::make_chart_contexts_current();
        // title_id() carries the "##id", so charts with the same title don't share state.
        if (!ImPlot3D::BeginPlot(title_id().c_str(), size(), legend_ ? flags_ : flags_ | ImPlot3DFlags_NoLegend)) {
            return false;
        }
        for (int id = 0; id < ImAxis3D_COUNT; ++id) {
            setup_axis(id, axes_[id]);
        }
        if (legend_) {
            ImPlot3D::SetupLegend(legend_->location, legend_->flags);
        }
        for (const auto& series : series_) {
            series->plot();
        }
        ImPlot3D::EndPlot();
        return false;
    }

    ImVec2 Chart3DWidget::measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) {
        // From the theme: the window's ImPlot3D context may not exist before its first chart is drawn.
        const ImVec2 intrinsic = chart_theme().plot3d.PlotDefaultSize;
        return ImVec2{
            fit_measure(intrinsic.x, width, widthMode),
            fit_measure(intrinsic.y, height, heightMode)
        };
    }
}
