// SeriesBuffer, SeriesView, SeriesRing and the SeriesData helpers. No ImGui or ImPlot context needed.

#include <gtest/gtest.h>

#include <span>
#include <vector>

#include <ironcast/extras/series.hpp>

using namespace ironcast;

namespace {
    std::vector<double> to_vector(std::span<const double> values) {
        return {values.begin(), values.end()};
    }

    // Ring contents oldest first, the order a chart plots them in.
    std::vector<double> chronological(const SeriesRing& ring) {
        const auto values = ring.values();
        std::vector<double> out;
        for (std::size_t i = 0; i < values.size(); ++i) {
            out.push_back(values[(ring.offset() + i) % values.size()]);
        }
        return out;
    }
}

TEST(SeriesBuffer, StartsEmpty) {
    const SeriesBuffer buffer;
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0u);
    EXPECT_TRUE(buffer.values().empty());
}

TEST(SeriesBuffer, AddsSingleValuesAndSpans) {
    SeriesBuffer buffer({1.0});
    const std::vector<double> more{3.0, 4.0};
    buffer.add(2.0).add(more);
    EXPECT_FALSE(buffer.empty());
    EXPECT_EQ(to_vector(buffer.values()), (std::vector<double>{1.0, 2.0, 3.0, 4.0}));
}

TEST(SeriesBuffer, RemoveKeepsOrder) {
    SeriesBuffer buffer({1.0, 2.0, 3.0, 4.0});
    buffer.remove(0).remove(1);
    EXPECT_EQ(to_vector(buffer.values()), (std::vector<double>{2.0, 4.0}));
    buffer.remove(1);
    EXPECT_EQ(to_vector(buffer.values()), (std::vector<double>{2.0}));
}

TEST(SeriesBuffer, Clear) {
    SeriesBuffer buffer({1.0, 2.0});
    buffer.clear();
    EXPECT_TRUE(buffer.empty());
    buffer.add(5.0);
    EXPECT_EQ(to_vector(buffer.values()), (std::vector<double>{5.0}));
}

TEST(SeriesView, BorrowsTheCallersBuffer) {
    std::vector<double> data{1.0, 2.0, 3.0};
    const SeriesView view(data);
    EXPECT_EQ(view.size(), 3u);
    EXPECT_FALSE(view.empty());
    EXPECT_EQ(view.values().data(), data.data());

    data[1] = 20.0;
    EXPECT_EQ(view.values()[1], 20.0);
}

TEST(SeriesView, DefaultIsEmpty) {
    const SeriesView view;
    EXPECT_TRUE(view.empty());
    EXPECT_EQ(view.size(), 0u);
}

TEST(SeriesRing, FillsUpWithoutWrapping) {
    SeriesRing ring(4);
    EXPECT_TRUE(ring.empty());
    EXPECT_EQ(ring.capacity(), 4u);

    ring.add(1.0).add(2.0).add(3.0);
    EXPECT_EQ(ring.size(), 3u);
    EXPECT_EQ(ring.offset(), 0u);
    EXPECT_EQ(to_vector(ring.values()), (std::vector<double>{1.0, 2.0, 3.0}));

    ring.add(4.0);
    EXPECT_EQ(ring.size(), 4u);
    EXPECT_EQ(ring.offset(), 0u);
}

TEST(SeriesRing, OverwritesOldestOnceFull) {
    SeriesRing ring(3);
    ring.add(1.0).add(2.0).add(3.0).add(4.0);
    EXPECT_EQ(ring.size(), 3u);
    EXPECT_EQ(ring.offset(), 1u);
    // storage order: the oldest slot was overwritten in place
    EXPECT_EQ(to_vector(ring.values()), (std::vector<double>{4.0, 2.0, 3.0}));
    EXPECT_EQ(chronological(ring), (std::vector<double>{2.0, 3.0, 4.0}));
}

TEST(SeriesRing, OffsetWrapsBackToZero) {
    SeriesRing ring(3);
    for (double v = 1.0; v <= 6.0; v += 1.0) {
        ring.add(v);
    }
    EXPECT_EQ(ring.offset(), 0u);
    EXPECT_EQ(to_vector(ring.values()), (std::vector<double>{4.0, 5.0, 6.0}));

    ring.add(7.0);
    EXPECT_EQ(ring.offset(), 1u);
    EXPECT_EQ(chronological(ring), (std::vector<double>{5.0, 6.0, 7.0}));
}

TEST(SeriesRing, KeepsLastCapacityValues) {
    SeriesRing ring(5);
    for (int i = 0; i < 1000; ++i) {
        ring.add(i);
    }
    EXPECT_EQ(ring.size(), 5u);
    EXPECT_EQ(chronological(ring), (std::vector<double>{995.0, 996.0, 997.0, 998.0, 999.0}));
}

TEST(SeriesRing, CapacityOneKeepsLatest) {
    SeriesRing ring(1);
    ring.add(1.0).add(2.0).add(3.0);
    EXPECT_EQ(ring.size(), 1u);
    EXPECT_EQ(ring.offset(), 0u);
    EXPECT_EQ(to_vector(ring.values()), (std::vector<double>{3.0}));
}

TEST(SeriesRing, ClearResetsOffset) {
    SeriesRing ring(3);
    ring.add(1.0).add(2.0).add(3.0).add(4.0);
    ring.clear();
    EXPECT_TRUE(ring.empty());
    EXPECT_EQ(ring.offset(), 0u);
    EXPECT_EQ(ring.capacity(), 3u);

    ring.add(9.0);
    EXPECT_EQ(to_vector(ring.values()), (std::vector<double>{9.0}));
}

TEST(SeriesData, ValuesAndOffsetForEachKind) {
    const std::vector<double> data{1.0, 2.0};
    const SeriesData buffer = SeriesBuffer({1.0, 2.0});
    const SeriesData view = SeriesView(data);

    SeriesRing ring(2);
    ring.add(1.0).add(2.0).add(3.0);
    const SeriesData ringData = ring;

    EXPECT_EQ(to_vector(values(buffer)), (std::vector<double>{1.0, 2.0}));
    EXPECT_EQ(offset(buffer), 0u);
    EXPECT_EQ(values(view).data(), data.data());
    EXPECT_EQ(offset(view), 0u);
    EXPECT_EQ(to_vector(values(ringData)), (std::vector<double>{3.0, 2.0}));
    EXPECT_EQ(offset(ringData), 1u);
}
