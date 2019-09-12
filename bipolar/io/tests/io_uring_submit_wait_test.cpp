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

const std::size_t BLOCKS = 4096;
const std::size_t PAGE_SIZE = 4096;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
TEST(IOUring, SubmitWait) {
    auto mem = std::aligned_alloc(PAGE_SIZE, PAGE_SIZE);
    EXPECT_NE(mem, nullptr);
    
    struct iovec iov = {
        .iov_base = mem,
        .iov_len = PAGE_SIZE,
    };

    struct io_uring_params p{
        .flags = IORING_SETUP_IOPOLL,
    };
    IOUring ring(4, &p);

    char buf[] = "./XXXXXX";
    int fd = mkostemp(buf, O_DIRECT);
    EXPECT_NE(fd, -1);

    BOOST_SCOPE_EXIT_ALL(&) {
        close(fd);
        unlink(buf);
    };
    

    off_t offset = 0;
    std::size_t blocks = BLOCKS;
    while (blocks--) {
        auto sub_res = ring.get_submission_entry();
        EXPECT_TRUE(bool(sub_res));

        IOUringSQE& sqe = sub_res.value();
        sqe.writev(fd, &iov, 1, offset);

        EXPECT_TRUE(bool(ring.submit(/* nr_wait = */ 1)));

        auto cpl_res = ring.get_completion_entry();
        EXPECT_TRUE(bool(cpl_res));

        IOUringCQE& cqe = cpl_res.value();
        if (cqe.res == -EOPNOTSUPP) {
            EXPECT_FALSE("Polling not supported in current directory");
            return;
        }
        EXPECT_EQ(cqe.res, PAGE_SIZE);

        ring.seen(1);
        offset += PAGE_SIZE;
    }
}
#endif
