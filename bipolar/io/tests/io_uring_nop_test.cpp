#include "bipolar/io/io_uring.hpp"

#include <linux/version.h>

#include <cstdio>

#include <gtest/gtest.h>

using namespace bipolar;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
TEST(IOUring, SingleNop) {
    struct io_uring_params p{};
    IOUring ring(8, &p);

    auto res = ring.get_submission_entry();
    EXPECT_TRUE(bool(res));

    res.value().get().nop();
    EXPECT_TRUE(bool(ring.submit()));
    EXPECT_TRUE(bool(ring.get_completion_entry()));
    ring.seen(1);
}

TEST(IOUring, BarrierNop) {
    struct io_uring_params p{};
    IOUring ring(8, &p);
    for (std::size_t i = 0; i < 8; ++i) {
        auto res = ring.get_submission_entry();
        EXPECT_TRUE(bool(res));

        IOUringSQE& sqe = res.value();
        sqe.nop();
        if (i == 4) {
            sqe.flags = IOSQE_IO_DRAIN;
        }
    }

    auto res = ring.submit();
    EXPECT_TRUE(bool(res));
    EXPECT_EQ(res.value(), 8);

    for (std::size_t i = 0; i < 8; ++i) {
        EXPECT_TRUE(bool(ring.get_completion_entry()));
        ring.seen(1);
    }
}
#endif
