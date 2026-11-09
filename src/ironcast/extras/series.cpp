#include <algorithm>
#include <format>

#include <implot.h>
#include "ironcast/assert.hpp"

#include "ironcast/extras/series.hpp"
#include "ironcast/ironcast.hpp"

namespace ironcast {
    SeriesBuffer::SeriesBuffer(std::vector<double> values): values_(std::move(values)) {}

    std::span<const double> SeriesBuffer::values() const {
        return values_;
    }

    std::size_t SeriesBuffer::size() const {
        return values_.size();
    }

    bool SeriesBuffer::empty() const {
        return values_.empty();
    }

    SeriesBuffer& SeriesBuffer::add(double value) {
        values_.push_back(value);
        return *this;
    }

    SeriesBuffer& SeriesBuffer::add(std::span<const double> values) {
        values_.insert(values_.end(), values.begin(), values.end());
        return *this;
    }

    SeriesBuffer& SeriesBuffer::remove(std::size_t index) {
        IRONCAST_DEBUG_ASSERT(index < values_.size(), "SeriesBuffer::remove index out of range", index, values_.size());
        values_.erase(values_.begin() + static_cast<std::ptrdiff_t>(index));
        return *this;
    }

    SeriesBuffer& SeriesBuffer::clear() {
        values_.clear();
        return *this;
    }

    SeriesView::SeriesView(std::span<const double> values): values_(values) {}

    std::span<const double> SeriesView::values() const {
        return values_;
    }

    std::size_t SeriesView::size() const {
        return values_.size();
    }

    bool SeriesView::empty() const {
        return values_.empty();
    }

    SeriesRing::SeriesRing(std::size_t capacity): capacity_(capacity) {
        IRONCAST_DEBUG_ASSERT(capacity > 0, "SeriesRing capacity must be > 0");
        values_.reserve(capacity);
    }

    std::span<const double> SeriesRing::values() const {
        return values_;
    }

    std::size_t SeriesRing::offset() const {
        return head_;
    }

    std::size_t SeriesRing::size() const {
        return values_.size();
    }

    std::size_t SeriesRing::capacity() const {
        return capacity_;
    }

    bool SeriesRing::empty() const {
        return values_.empty();
    }

    SeriesRing& SeriesRing::add(double value) {
        if (values_.size() < capacity_) {
            values_.push_back(value);
        } else {
            values_[head_] = value;
            head_ = (head_ + 1) % capacity_;
        }
        return *this;
    }

    SeriesRing& SeriesRing::clear() {
        values_.clear();
        head_ = 0;
        return *this;
    }

    std::span<const double> values(const SeriesData& data) {
        return std::visit([](const auto& d) { return d.values(); }, data);
    }

    std::size_t offset(const SeriesData& data) {
        const auto* ring = std::get_if<SeriesRing>(&data);
        return ring ? ring->offset() : 0;
    }

    Series::Series(const std::string& label)
    : id_(Ironcast::random()), label_(label), labelID_(std::format("{}##{}", label, id_)) {}

    uint64_t Series::id() const {
        return id_;
    }

    const std::string& Series::label() const {
        return label_;
    }

    const std::string& Series::label_id() const {
        return labelID_;
    }

    Series& Series::set_label(const std::string& label) {
        label_ = label;
        labelID_ = std::format("{}##{}", label, id_);
        return *this;
    }

    std::optional<ImVec4> Series::color() const {
        return color_;
    }

    Series& Series::set_color(std::optional<ImVec4> color) {
        color_ = color;
        return *this;
    }

    ImAxis Series::x_axis() const {
        return xAxis_;
    }

    ImAxis Series::y_axis() const {
        return yAxis_;
    }

    Series& Series::set_axes(ImAxis x, ImAxis y) {
        IRONCAST_DEBUG_ASSERT(x >= ImAxis_X1 && x <= ImAxis_X3 && y >= ImAxis_Y1 && y <= ImAxis_Y3, "Series::set_axes: x must be X1..X3, y Y1..Y3", x, y);
        xAxis_ = x;
        yAxis_ = y;
        return *this;
    }

    XYSeries::XYSeries(const std::string& label, SeriesData xs, SeriesData ys)
    : Series(label), xs_(std::move(xs)), ys_(std::move(ys)) {}

    XYSeries& XYSeries::set_data(SeriesData xs, SeriesData ys) {
        xs_ = std::move(xs);
        ys_ = std::move(ys);
        return *this;
    }

    SeriesData& XYSeries::xs_data() {
        return xs_;
    }

    SeriesData& XYSeries::ys_data() {
        return ys_;
    }

    std::span<const double> XYSeries::xs() const {
        return values(xs_);
    }

    std::span<const double> XYSeries::ys() const {
        return values(ys_);
    }

    int XYSeries::count() const {
        return static_cast<int>(std::min(values(xs_).size(), values(ys_).size()));
    }

    int XYSeries::offset() const {
        IRONCAST_DEBUG_ASSERT(ironcast::offset(xs_) == ironcast::offset(ys_), "XYSeries: xs and ys rings out of step", ironcast::offset(xs_), ironcast::offset(ys_));
        return static_cast<int>(ironcast::offset(xs_));
    }

    float LineSeries::line_weight() const {
        return lineWeight_;
    }

    LineSeries& LineSeries::set_line_weight(float weight) {
        lineWeight_ = weight;
        return *this;
    }

    void LineSeries::plot() const {
        ImPlotSpec spec;
        spec.LineColor = color().value_or(IMPLOT_AUTO_COL);
        spec.LineWeight = lineWeight_;
        spec.Offset = offset();
        ImPlot::PlotLine(label_id().c_str(), xs().data(), ys().data(), count(), spec);
    }

    float ScatterSeries::line_weight() const {
        return lineWeight_;
    }

    ScatterSeries& ScatterSeries::set_line_weight(float weight) {
        lineWeight_ = weight;
        return *this;
    }

    void ScatterSeries::plot() const {
        // Marker fill and edge default to LineColor.
        ImPlotSpec spec;
        spec.LineColor = color().value_or(IMPLOT_AUTO_COL);
        spec.LineWeight = lineWeight_;
        spec.Offset = offset();
        ImPlot::PlotScatter(label_id().c_str(), xs().data(), ys().data(), count(), spec);
    }

    double BarSeries::bar_width() const {
        return barWidth_;
    }

    BarSeries& BarSeries::set_bar_width(double width) {
        barWidth_ = width;
        return *this;
    }

    void BarSeries::plot() const {
        ImPlotSpec spec;
        spec.FillColor = color().value_or(IMPLOT_AUTO_COL);
        spec.Offset = offset();
        ImPlot::PlotBars(label_id().c_str(), xs().data(), ys().data(), count(), barWidth_, spec);
    }
}
