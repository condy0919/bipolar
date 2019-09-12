#include "bipolar/io/io_uring.hpp"

#include <linux/version.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstdint>

#include <gtest/gtest.h>

using namespace bipolar;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
TEST(IOUring, SingleLink) {
    struct io_uring_params p{
        .flags = IORING_SETUP_IOPOLL,
    };

    IOUring ring(8, &p);

    auto sub_res = ring.get_submission_entry();
    EXPECT_TRUE(bool(sub_res));

    IOUringSQE& sqe = sub_res.value();
    sqe.nop();
    sqe.flags |= IOSQE_IO_LINK;

    sub_res = ring.get_submission_entry();
    EXPECT_TRUE(bool(sub_res));

    IOUringSQE& sqe2 = sub_res.value();
    sqe2.nop();

    auto res = ring.submit();
    EXPECT_TRUE(bool(res));

    for (std::size_t i = 0; i < 2; ++i) {
        auto cpl_res = ring.get_completion_entry();
        EXPECT_TRUE(bool(cpl_res));

        ring.seen(1);
    }
}

TEST(IOUring, DoubleLink) {
    struct io_uring_params p{
        .flags = IORING_SETUP_IOPOLL,
    };

    IOUring ring(8, &p);

    auto sub_res = ring.get_submission_entry();
    EXPECT_TRUE(bool(sub_res));

    IOUringSQE& sqe = sub_res.value();
    sqe.nop();
    sqe.flags |= IOSQE_IO_LINK;

    sub_res = ring.get_submission_entry();
    EXPECT_TRUE(bool(sub_res));

    IOUringSQE& sqe2 = sub_res.value();
    sqe2.nop();
    sqe2.flags |= IOSQE_IO_LINK;

    sub_res = ring.get_submission_entry();
    EXPECT_TRUE(bool(sub_res));

    IOUringSQE& sqe3 = sub_res.value();
    sqe3.nop();

    auto res = ring.submit();
    EXPECT_TRUE(bool(res));

    for (std::size_t i = 0; i < 3; ++i) {
        auto cpl_res = ring.get_completion_entry();
        EXPECT_TRUE(bool(cpl_res));

        ring.seen(1);
    }
}

TEST(IOUring, DoubleChain) {
    struct io_uring_params p{
        .flags = IORING_SETUP_IOPOLL,
    };

    IOUring ring(8, &p);

    auto sub_res = ring.get_submission_entry();
    EXPECT_TRUE(bool(sub_res));

    IOUringSQE& sqe = sub_res.value();
    sqe.nop();
    sqe.flags |= IOSQE_IO_LINK;

    sub_res = ring.get_submission_entry();
    EXPECT_TRUE(bool(sub_res));

    IOUringSQE& sqe2 = sub_res.value();
    sqe2.nop();

    sub_res = ring.get_submission_entry();
    EXPECT_TRUE(bool(sub_res));

    IOUringSQE& sqe3 = sub_res.value();
    sqe3.nop();
    sqe3.flags |= IOSQE_IO_LINK;

    sub_res = ring.get_submission_entry();
    EXPECT_TRUE(bool(sub_res));

    IOUringSQE& sqe4 = sub_res.value();
    sqe4.nop();

    auto res = ring.submit();
    EXPECT_TRUE(bool(res));

    for (std::size_t i = 0; i < 4; ++i) {
        auto cpl_res = ring.get_completion_entry();
        EXPECT_TRUE(bool(cpl_res));

        ring.seen(1);
    }
}

TEST(IOUring, SingleLinkFail) {
    struct io_uring_params p{
        .flags = IORING_SETUP_IOPOLL,
    };

    IOUring ring(8, &p);

    auto sub_res = ring.get_submission_entry();
    EXPECT_TRUE(bool(sub_res));

    IOUringSQE& sqe = sub_res.value();
    sqe.nop();
    sqe.flags |= IOSQE_IO_LINK;

    auto sub_res2 = ring.get_submission_entry();
    EXPECT_TRUE(bool(sub_res2));

    IOUringSQE& sqe2 = sub_res2.value();
    sqe2.nop();

    auto res = ring.submit();
    EXPECT_TRUE(bool(res));

    for (std::size_t i = 0; i < 2; ++i) {
        auto cpl_res = ring.peek_completion_entry();
        EXPECT_TRUE(bool(cpl_res));

        IOUringCQE& cqe = cpl_res.value();

        if (i == 0) {
            EXPECT_EQ(cqe.res, -EINVAL);
        } else if (i == 1) {
            EXPECT_EQ(cqe.res, -ECANCELED);
        }

        ring.seen(1);
    }
}
#endif
