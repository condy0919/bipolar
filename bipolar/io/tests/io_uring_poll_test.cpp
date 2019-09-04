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

static void polling(int fd) {
    struct io_uring_params p{};
    IOUring ring(1, &p);

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
    sqe.poll_add(fd, POLLIN);
    sqe.user_data = (std::uint64_t)std::addressof(sqe);

    auto res = ring.submit();
    EXPECT_TRUE(bool(res));

    auto cpl_res = ring.get_completion_entry();
    EXPECT_TRUE(bool(cpl_res));
    ring.seen(1);

    IOUringCQE& cqe = cpl_res.value();
    EXPECT_EQ(cqe.user_data, (std::uint64_t)std::addressof(sqe));
    EXPECT_EQ(cqe.res & POLLIN, POLLIN);
}

TEST(IOUring, Poll) {
    int pipe1[2];
    EXPECT_EQ(pipe(pipe1), 0);

    int ret = 0;
    pid_t pid = fork();
    switch (pid) {
    case -1:
        EXPECT_TRUE(false) << "fork failed";
        break;

    case 0:
        polling(pipe1[0]);
        break;

    default:
        do {
            errno = 0;
            ret = write(pipe1[1], "foo", 3);
        } while (ret == -1 && errno == EINTR);
        EXPECT_EQ(ret, 3);
    }
}
