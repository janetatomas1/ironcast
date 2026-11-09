#pragma once

#include <array>
#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <implot3d.h>

#include "ironcast/extras/charttheme.hpp"
#include "ironcast/extras/series3d.hpp"
#include "ironcast/widget.hpp"

namespace ironcast {
    // One 3D plot axis, applied in ImPlot3D's Setup phase every frame.
    struct Axis3D {
        // Empty = no label.
        std::string label;
        ImPlot3DAxisFlags flags = ImPlot3DAxisFlags_None;
        // Fixed limits; empty = auto-fit to the data.
        std::optional<ImPlot3DRange> limits;
        ImPlot3DScale scale = ImPlot3DScale_Linear;
    };

    // The 3D plot legend, applied in ImPlot3D's Setup phase every frame.
    struct Legend3D {
        ImPlot3DLocation location = ImPlot3DLocation_NorthWest;
        ImPlot3DLegendFlags flags = ImPlot3DLegendFlags_None;
    };

    // An ImPlot3D plot drawn at this widget's Yoga box; the 3D counterpart of ChartWidget.
    // It owns its Series3D. Intrinsic size is ImPlot3D's PlotDefaultSize.
    // The title is the plot title; "##" hides it.
    class Chart3DWidget : public Widget {
        std::vector<std::unique_ptr<Series3D>> series_;
        // Indexed by ImAxis3D.
        std::array<Axis3D, ImAxis3D_COUNT> axes_;
        std::optional<Legend3D> legend_ = Legend3D{};
        ImPlot3DFlags flags_ = ImPlot3DFlags_None;

    public:
        Chart3DWidget(const std::string& title = "", Widget* parent = nullptr);

        // The reference stays valid until the series is removed or the chart is destroyed.
        template <std::derived_from<Series3D> S, class... Args>
        S& add_series(Args&&... args) {
            series_.push_back(std::make_unique<S>(std::forward<Args>(args)...));
            return static_cast<S&>(*series_.back());
        }
        Series3D& add_series(std::unique_ptr<Series3D> series);
        // Destroys the series; does nothing when it isn't in this chart.
        void remove_series(const Series3D& series);
        // Destroys every series with this label.
        void remove_series(const std::string& label);
        // Destroys the series with this id; does nothing when it isn't in this chart.
        void remove_series(uint64_t id);
        // The first series with this label; nullptr when there is none.
        [[nodiscard]] Series3D* get_series(const std::string& label);
        // The series with this id; nullptr when it isn't in this chart.
        [[nodiscard]] Series3D* get_series(uint64_t id);
        void clear_series();
        [[nodiscard]] const std::vector<std::unique_ptr<Series3D>>& series() const;

        [[nodiscard]] Axis3D& axis(ImAxis3D id);
        [[nodiscard]] const Axis3D& axis(ImAxis3D id) const;
        Chart3DWidget& set_axis(ImAxis3D id, Axis3D axis);

        // Shown by default; empty = no legend.
        [[nodiscard]] std::optional<Legend3D>& legend();
        [[nodiscard]] const std::optional<Legend3D>& legend() const;
        Chart3DWidget& set_legend(std::optional<Legend3D> legend);

        [[nodiscard]] ImPlot3DFlags flags() const;
        Chart3DWidget& set_flags(ImPlot3DFlags flags);

        bool show() final;
        ImVec2 measure(float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode) final;
    };
}
