#define private public
#include "bipolar/io/io_uring.hpp"
#undef private

#include <cstdio>

#include <gtest/gtest.h>

using namespace bipolar;

static void queue_n_nops(IOUring& ring, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        auto res = ring.get_submission_entry();
        EXPECT_TRUE(bool(res));
        res.value().get().nop();
    }

    auto res = ring.submit();
    EXPECT_TRUE(bool(res));
    EXPECT_EQ(res.value(), n);
}

TEST(IOUring, CqFull) {
    struct io_uring_params p{};
    IOUring ring(4, &p);

    queue_n_nops(ring, 4);
    queue_n_nops(ring, 4);
    queue_n_nops(ring, 4);

    std::size_t i = 0;
    while (auto res = ring.peek_completion_entry()) {
        ring.seen(1);
        ++i;
    }

    EXPECT_EQ(i, 8);
    EXPECT_EQ(*ring.cq_.koverflow_, 4);
}
