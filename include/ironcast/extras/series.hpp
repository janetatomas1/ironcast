#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

#include <imgui.h>
#include <implot.h>

namespace ironcast {
    // Values a series owns.
    class SeriesBuffer {
        std::vector<double> values_;

    public:
        SeriesBuffer() = default;
        SeriesBuffer(std::vector<double> values);

        [[nodiscard]] std::span<const double> values() const;
        [[nodiscard]] std::size_t size() const;
        [[nodiscard]] bool empty() const;

        SeriesBuffer& add(double value);
        SeriesBuffer& add(std::span<const double> values);
        // Index must be < size().
        SeriesBuffer& remove(std::size_t index);
        SeriesBuffer& clear();
    };

    // Values a series borrows: the caller keeps the buffer alive while the series uses it,
    // and changes it only on the UI thread (post()), never during a frame.
    class SeriesView {
        std::span<const double> values_;

    public:
        SeriesView() = default;
        SeriesView(std::span<const double> values);

        [[nodiscard]] std::span<const double> values() const;
        [[nodiscard]] std::size_t size() const;
        [[nodiscard]] bool empty() const;
    };

    // The last capacity values a series owns: once full, add() overwrites the oldest.
    // values() is in storage order; the oldest value is at offset().
    class SeriesRing {
        std::vector<double> values_;
        std::size_t capacity_;
        std::size_t head_ = 0;

    public:
        // capacity > 0.
        explicit SeriesRing(std::size_t capacity);

        [[nodiscard]] std::span<const double> values() const;
        [[nodiscard]] std::size_t offset() const;
        [[nodiscard]] std::size_t size() const;
        [[nodiscard]] std::size_t capacity() const;
        [[nodiscard]] bool empty() const;

        SeriesRing& add(double value);
        SeriesRing& clear();
    };

    // Values a series plots: owned, borrowed, or a ring.
    using SeriesData = std::variant<SeriesBuffer, SeriesView, SeriesRing>;
    [[nodiscard]] std::span<const double> values(const SeriesData& data);
    // Index of the oldest value: SeriesRing::offset(), 0 for the others.
    [[nodiscard]] std::size_t offset(const SeriesData& data);

    // One plot item of a ChartWidget. Subclasses draw themselves in plot(),
    // which runs between ImPlot::BeginPlot and EndPlot.
    class Series {
        uint64_t id_;
        std::string label_;
        std::string labelID_;
        std::optional<ImVec4> color_;
        ImAxis xAxis_ = ImAxis_X1;
        ImAxis yAxis_ = ImAxis_Y1;

    public:
        explicit Series(const std::string& label);
        virtual ~Series() = default;

        virtual void plot() const = 0;

        // Unique per series, fixed for its lifetime.
        [[nodiscard]] uint64_t id() const;
        // Legend entry; "##" hides it, as in ImGui.
        [[nodiscard]] const std::string& label() const;
        // label() with "##id": the ImPlot item id, so series with the same label don't share state.
        [[nodiscard]] const std::string& label_id() const;
        Series& set_label(const std::string& label);
        // Empty = ImPlot picks the next colormap color.
        [[nodiscard]] std::optional<ImVec4> color() const;
        Series& set_color(std::optional<ImVec4> color);
        // The chart axes this series is drawn against: x one of ImAxis_X1..X3, y one of ImAxis_Y1..Y3.
        [[nodiscard]] ImAxis x_axis() const;
        [[nodiscard]] ImAxis y_axis() const;
        Series& set_axes(ImAxis x, ImAxis y);
    };

    // A series of (x, y) points; xs and ys are plotted up to the shorter of the two.
    // Rings for xs and ys must stay in step (same capacity, added together) so they share one offset.
    class XYSeries : public Series {
        SeriesData xs_;
        SeriesData ys_;

    protected:
        [[nodiscard]] int count() const;
        // The shared offset of xs and ys, for ImPlotSpec::Offset.
        [[nodiscard]] int offset() const;

    public:
        XYSeries(const std::string& label, SeriesData xs, SeriesData ys);
        XYSeries& set_data(SeriesData xs, SeriesData ys);
        // E.g. std::get_if<SeriesBuffer>(&series.xs_data()) to add or remove owned values.
        [[nodiscard]] SeriesData& xs_data();
        [[nodiscard]] SeriesData& ys_data();

        // The values whatever the data type, in storage order (see offset()).
        [[nodiscard]] std::span<const double> xs() const;
        [[nodiscard]] std::span<const double> ys() const;
        // The data as T, e.g. xs<SeriesRing>().add(t); T must be the type it holds.
        template <class T>
        [[nodiscard]] T& xs() { return std::get<T>(xs_); }
        template <class T>
        [[nodiscard]] const T& xs() const { return std::get<T>(xs_); }
        template <class T>
        [[nodiscard]] T& ys() { return std::get<T>(ys_); }
        template <class T>
        [[nodiscard]] const T& ys() const { return std::get<T>(ys_); }
    };

    // Points joined by straight lines (ImPlot::PlotLine).
    class LineSeries : public XYSeries {
        float lineWeight_ = 1.0f;

    public:
        using XYSeries::XYSeries;
        // Line thickness in pixels.
        [[nodiscard]] float line_weight() const;
        LineSeries& set_line_weight(float weight);
        void plot() const override;
    };

    // Points drawn as markers, unconnected (ImPlot::PlotScatter).
    class ScatterSeries : public XYSeries {
        float lineWeight_ = 1.0f;

    public:
        using XYSeries::XYSeries;
        // Marker edge thickness in pixels.
        [[nodiscard]] float line_weight() const;
        ScatterSeries& set_line_weight(float weight);
        void plot() const override;
    };

    // Bars centered on xs, barWidth in plot units.
    class BarSeries : public XYSeries {
        double barWidth_ = 0.67;

    public:
        using XYSeries::XYSeries;
        [[nodiscard]] double bar_width() const;
        BarSeries& set_bar_width(double width);
        void plot() const override;
    };
}
