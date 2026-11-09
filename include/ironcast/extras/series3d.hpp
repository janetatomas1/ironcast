#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>

#include <imgui.h>

#include "ironcast/extras/series.hpp"

namespace ironcast {
    // One plot item of a Chart3DWidget. Subclasses draw themselves in plot(),
    // which runs between ImPlot3D::BeginPlot and EndPlot.
    class Series3D {
        uint64_t id_;
        std::string label_;
        std::string labelID_;
        std::optional<ImVec4> color_;

    public:
        explicit Series3D(const std::string& label);
        virtual ~Series3D() = default;

        virtual void plot() const = 0;

        // Unique per series, fixed for its lifetime.
        [[nodiscard]] uint64_t id() const;
        // Legend entry; "##" hides it, as in ImGui.
        [[nodiscard]] const std::string& label() const;
        // label() with "##id": the ImPlot3D item id, so series with the same label don't share state.
        [[nodiscard]] const std::string& label_id() const;
        Series3D& set_label(const std::string& label);
        // Empty = ImPlot3D picks the next colormap color.
        [[nodiscard]] std::optional<ImVec4> color() const;
        Series3D& set_color(std::optional<ImVec4> color);
    };

    // A series of (x, y, z) points; plotted up to the shortest of the three.
    // Rings must stay in step (same capacity, added together) so they share one offset.
    class XYZSeries : public Series3D {
        SeriesData xs_;
        SeriesData ys_;
        SeriesData zs_;

    protected:
        [[nodiscard]] int count() const;
        // The shared offset of xs, ys and zs, for ImPlot3DSpec::Offset.
        [[nodiscard]] int offset() const;

    public:
        XYZSeries(const std::string& label, SeriesData xs, SeriesData ys, SeriesData zs);
        XYZSeries& set_data(SeriesData xs, SeriesData ys, SeriesData zs);
        [[nodiscard]] SeriesData& xs_data();
        [[nodiscard]] SeriesData& ys_data();
        [[nodiscard]] SeriesData& zs_data();

        // The values whatever the data type, in storage order (see offset()).
        [[nodiscard]] std::span<const double> xs() const;
        [[nodiscard]] std::span<const double> ys() const;
        [[nodiscard]] std::span<const double> zs() const;
        // The data as T, e.g. xs<SeriesRing>().add(t); T must be the type it holds.
        template <class T>
        [[nodiscard]] T& xs() { return std::get<T>(xs_); }
        template <class T>
        [[nodiscard]] const T& xs() const { return std::get<T>(xs_); }
        template <class T>
        [[nodiscard]] T& ys() { return std::get<T>(ys_); }
        template <class T>
        [[nodiscard]] const T& ys() const { return std::get<T>(ys_); }
        template <class T>
        [[nodiscard]] T& zs() { return std::get<T>(zs_); }
        template <class T>
        [[nodiscard]] const T& zs() const { return std::get<T>(zs_); }
    };

    // Points joined by straight lines in 3D (ImPlot3D::PlotLine).
    class Line3DSeries : public XYZSeries {
        float lineWeight_ = 1.0f;

    public:
        using XYZSeries::XYZSeries;
        // Line thickness in pixels.
        [[nodiscard]] float line_weight() const;
        Line3DSeries& set_line_weight(float weight);
        void plot() const override;
    };

    // Points drawn as markers in 3D, unconnected (ImPlot3D::PlotScatter).
    class Scatter3DSeries : public XYZSeries {
        float lineWeight_ = 1.0f;

    public:
        using XYZSeries::XYZSeries;
        // Marker edge thickness in pixels.
        [[nodiscard]] float line_weight() const;
        Scatter3DSeries& set_line_weight(float weight);
        void plot() const override;
    };
}
