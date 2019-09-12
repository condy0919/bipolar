#include "bipolar/io/io_uring.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstdint>

#include <boost/scope_exit.hpp>
#include <gtest/gtest.h>

using namespace bipolar;

namespace {
const std::size_t PAGE_SIZE = 4096;
const char* FILENAME = "testfile";
}

TEST(IOUring, SingleFsync) {
    struct io_uring_params p{};

    IOUring ring(8, &p);

    char buf[] = "./XXXXXX";
    int fd = mkstemp(buf);
    EXPECT_GE(fd, 0);

    BOOST_SCOPE_EXIT_ALL(buf) {
        unlink(buf);
    };

    auto sub_res = ring.get_submission_entry();
    EXPECT_TRUE(bool(sub_res));

    IOUringSQE& sqe = sub_res.value();
    sqe.fsync(fd, 0);

    auto res = ring.submit();
    EXPECT_TRUE(bool(res));
    EXPECT_GT(res.value(), 0);

    auto cpl_res = ring.get_completion_entry();
    EXPECT_TRUE(bool(cpl_res));

    ring.seen(1);
}

TEST(IOUring, BarrierFsync) {
    struct io_uring_params p{};

    IOUring ring(8, &p);

    int fd = open(FILENAME, O_WRONLY | O_CREAT, 0644);
    EXPECT_GE(fd, 0);

    BOOST_SCOPE_EXIT_ALL() {
        unlink(FILENAME);
    };

    struct iovec iovecs[4];
    for (auto& iov : iovecs) {
        auto mem = std::aligned_alloc(PAGE_SIZE, PAGE_SIZE);
        EXPECT_NE(mem, nullptr);

        iov.iov_base = mem;
        iov.iov_len = PAGE_SIZE;
    }

    off_t off = 0;
    for (auto& iov : iovecs) {
        auto sub_res = ring.get_submission_entry();
        EXPECT_TRUE(bool(sub_res));

        IOUringSQE& sqe = sub_res.value();
        sqe.writev(fd, std::addressof(iov), 1, off);
        sqe.user_data = 0;

        off += PAGE_SIZE;
    }

    auto sub_res = ring.get_submission_entry();
    EXPECT_TRUE(bool(sub_res));

    IOUringSQE& sqe = sub_res.value();
    sqe.fsync(fd, IORING_FSYNC_DATASYNC);
    sqe.user_data = 1;
    sqe.flags = IOSQE_IO_DRAIN;

    auto res = ring.submit();
    EXPECT_TRUE(bool(res));
    EXPECT_EQ(res.value(), 4 + 1);

    for (std::size_t i = 0; i < 4 + 1; ++i) {
        auto cpl_res = ring.get_completion_entry();
        EXPECT_TRUE(bool(cpl_res));

        IOUringCQE& cqe = cpl_res.value();

        // Kernel doesn't support IOSQE_IO_DRAIN
        if (cqe.res == -EINVAL) {
            break;
        }

        if (i <= 3) {
            EXPECT_EQ(cqe.user_data, 0);
        } else {
            EXPECT_EQ(cqe.user_data, 1);
        }

        ring.seen(1);
    }
}
