#include "bipolar/async/pipe.hpp"

#include <gtest/gtest.h>

TEST(Pipe, Pipable) {
    struct Foo : bipolar::Pipable {
        Foo(int v) : value(v) {}

        int value;
    };

    const int result = Foo(10) | [](const Foo& foo) {
        EXPECT_EQ(foo.value, 10);
        return Foo(foo.value * 2);
    } | [](const Foo& foo) {
        EXPECT_EQ(foo.value, 20);
        return -1;
    };
    EXPECT_EQ(result, -1);
}
