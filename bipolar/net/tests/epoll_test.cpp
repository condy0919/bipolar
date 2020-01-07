#include "bipolar/net/epoll.hpp"

#include <gtest/gtest.h>

using namespace bipolar;

TEST(epoll, stringify_interests) {
    EXPECT_EQ(stringify_interests(0), "");
    EXPECT_EQ(stringify_interests(EPOLLIN | EPOLLOUT), "IN OUT ");
    EXPECT_EQ(stringify_interests(EPOLLOUT | EPOLLIN), "IN OUT ");
    EXPECT_EQ(stringify_interests(EPOLLIN), "IN ");
    EXPECT_EQ(stringify_interests(EPOLLOUT), "OUT ");
    EXPECT_EQ(stringify_interests(EPOLLRDHUP), "RDHUP ");
    EXPECT_EQ(stringify_interests(EPOLLPRI), "PRI ");
    EXPECT_EQ(stringify_interests(EPOLLERR), "ERR ");
    EXPECT_EQ(stringify_interests(EPOLLHUP), "HUP ");
}
