#include "bipolar/io/io_uring.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <wait.h>

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <csignal>

#include <gtest/gtest.h>

using namespace bipolar;

struct PollData {
    bool is_poll;
    bool is_cancel;
};

TEST(IOUring, PollCancel) {
    int pipe1[2];
    EXPECT_EQ(pipe(pipe1), 0);

    PollData pds[2];

    struct io_uring_params p{};
    IOUring ring(2, &p);

    struct sigaction act{};
    act.sa_handler = [](int sig) {
        (void)sig;
        EXPECT_TRUE(false) << "timeout";
    };
    act.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &act, nullptr);
    alarm(1);

    auto sub_res = ring.get_submission_entry();
    EXPECT_TRUE(bool(sub_res));

    IOUringSQE& sqe = sub_res.value();
    sqe.poll_add(pipe1[0], POLLIN);

    pds[0].is_poll = true;
    pds[0].is_cancel = false;
    sqe.user_data = (std::uint64_t)&pds[0];

    auto res = ring.submit();
    EXPECT_TRUE(bool(res));

    sub_res = ring.get_submission_entry();
    EXPECT_TRUE(bool(sub_res));

    pds[1].is_poll = false;
    pds[1].is_cancel = false;

    IOUringSQE& sqe2 = sub_res.value();
    sqe2.poll_remove(&pds[0]);
    sqe2.user_data = (std::uint64_t)&pds[1];

    res = ring.submit();
    EXPECT_TRUE(bool(res));

    auto cpl_res = ring.get_completion_entry();
    EXPECT_TRUE(bool(cpl_res));

    IOUringCQE& cqe = cpl_res.value();
    auto pd = (PollData*)cqe.user_data;
    EXPECT_EQ(cqe.res, 0) << "sqe ("
                          << "add=" << pd->is_poll << "/"
                          << "remove=" << pd->is_cancel << ")"
                          << "failed with " << cqe.res;
    ring.seen(1);

    cpl_res = ring.get_completion_entry();
    EXPECT_TRUE(bool(cpl_res));

    IOUringCQE& cqe2 = cpl_res.value();
    pd = (PollData*)cqe2.user_data;
    EXPECT_EQ(cqe2.res, 0) << "sqe ("
                           << "add=" << pd->is_poll << "/"
                           << "remove=" << pd->is_cancel << ")"
                           << "failed with " << cqe2.res;
    ring.seen(1);
}
