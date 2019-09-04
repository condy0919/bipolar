#include "bipolar/io/io_uring.hpp"

#include <cstdio>

#include <gtest/gtest.h>

using namespace bipolar;

TEST(IOUring, SqFull) {
    struct io_uring_params p{};
    IOUring ring(8, &p);

    std::size_t i = 0;
    while (auto res = ring.get_submission_entry()) {
        ++i;
    }
    EXPECT_EQ(i, 8);
}
