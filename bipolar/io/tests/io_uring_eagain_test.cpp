#include "bipolar/io/io_uring.hpp"

#include <linux/version.h>
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
const char EMPTY[PAGE_SIZE] = "";
}

static int get_file_fd() {
    int fd = open(FILENAME, O_RDWR | O_CREAT, 0644);
    EXPECT_GE(fd, 0);

    int ret = write(fd, EMPTY, sizeof(EMPTY));
    EXPECT_EQ(ret, PAGE_SIZE);

    fsync(fd);

    ret = posix_fadvise(fd, 0, PAGE_SIZE, POSIX_FADV_DONTNEED);
    EXPECT_EQ(ret, 0);

    return fd;
}

static void close_file_fd(int fd) {
    close(fd);
    unlink(FILENAME);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
TEST(IOUring, Eagain) {
    auto mem = std::aligned_alloc(PAGE_SIZE, PAGE_SIZE);
    EXPECT_NE(mem, nullptr);

    struct iovec iov = {
        .iov_base = mem,
        .iov_len = PAGE_SIZE,
    };

    struct io_uring_params p{};

    IOUring ring(2, &p);
    auto sub_res = ring.get_submission_entry();
    EXPECT_TRUE(bool(sub_res));

    int fd = get_file_fd();
    BOOST_SCOPE_EXIT_ALL(fd) {
        close_file_fd(fd);
    };

    IOUringSQE& sqe = sub_res.value();
    sqe.readv(fd, &iov, 1, 0);
    sqe.rw_flags = RWF_NOWAIT;

    auto res = ring.submit();
    EXPECT_TRUE(bool(res));
    EXPECT_EQ(res.value(), 1);

    auto peek_res = ring.peek_completion_entry();
    EXPECT_TRUE(bool(peek_res));

    IOUringCQE& cqe = peek_res.value();
    EXPECT_EQ(cqe.res, -EAGAIN);
}
#endif
