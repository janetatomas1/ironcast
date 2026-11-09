// Container: removing a widget runs its destroy() before freeing it.

#include <gtest/gtest.h>

#include <ironcast/container.hpp>
#include <ironcast/widget.hpp>

using namespace ironcast;

namespace {
    struct Counted : Widget {
        int* destroyed;

        explicit Counted(int* destroyed) : destroyed(destroyed) {}

        void destroy() override {
            ++*destroyed;
            Widget::destroy();
        }
    };
}

TEST(Container, RemoveDestroysTheWidget) {
    Container<Widget> container;
    int first = 0;
    int second = 0;
    container.add<Counted>(&first);
    container.add<Counted>(&second);

    container.remove(0);
    EXPECT_EQ(first, 1);
    EXPECT_EQ(second, 0);
    EXPECT_EQ(container.count(), 1u);
}
