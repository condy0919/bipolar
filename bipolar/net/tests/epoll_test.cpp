#include "bipolar/net/epoll.hpp"
#include "bipolar/net/tcp.hpp"
#include "bipolar/net/udp.hpp"

#include <chrono>
#include <gtest/gtest.h>

using namespace bipolar;

TEST(Epoll, run_once_with_nothing) {
    auto epoll = Epoll::create().value();

    std::vector<struct epoll_event> events(10);
    auto poll_result = epoll.poll(events, std::chrono::milliseconds(10));
    EXPECT_TRUE(poll_result.is_ok());
    EXPECT_EQ(events.size(), 0);
}

TEST(Epoll, add_then_close) {
    auto epoll = Epoll::create().value();
    auto listener = TcpListener::bind(SocketAddress(IPv4Address(), 0)).value();

    epoll.add(listener.as_fd(), 0, EPOLLIN | EPOLLOUT);
    listener.close();

    std::vector<struct epoll_event> events(10);
    auto epoll_result = epoll.poll(events, std::chrono::milliseconds(100));
    EXPECT_TRUE(epoll_result.is_ok());
    EXPECT_EQ(events.size(), 0);
}

TEST(Epoll, mod) {
    auto epoll = Epoll::create().value();
    auto udp_socket = UdpSocket::bind(SocketAddress(IPv4Address(), 0)).value();

    epoll.add(udp_socket.as_fd(), 0, EPOLLIN);
    epoll.mod(udp_socket.as_fd(), 0, EPOLLIN);
    epoll.mod(udp_socket.as_fd(), 0, EPOLLOUT);

    std::vector<struct epoll_event> events(10);
    epoll.poll(events, std::chrono::milliseconds(-1));
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].events, EPOLLOUT);
}

TEST(Epoll, stringify_interests) {
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
