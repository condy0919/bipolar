#include <iostream>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <queue>
#include <thread>
#include <tuple>
#include <vector>

#include "bipolar/core/byteorder.hpp"
#include "bipolar/core/function.hpp"
#include "bipolar/net/epoll.hpp"
#include "bipolar/net/tcp.hpp"
#include "bipolar/sync/barrier.hpp"

#include <gtest/gtest.h>

using namespace bipolar;
using namespace std::string_literals;

// TODO more tests

inline constexpr SocketAddress
    server_addr(IPv4Address(127, 0, 0, 1),
                hton(static_cast<std::uint16_t>(8081)));

const auto LISTENER_MAGIC_NUMBER =
    reinterpret_cast<void*>(static_cast<std::uintptr_t>(0x50043));

TEST(TcpListener, bind_and_accept) {
    auto epoll = Epoll::create().expect("epoll_create failed");
    std::vector<struct epoll_event> events(10);

    auto listener =
        TcpListener::bind(SocketAddress(IPv4Address(127, 0, 0, 1), 0))
            .expect("bind to 127.0.0.1:0 failed");
    auto server_addr = listener.local_addr().value();

    epoll.add(listener.as_fd(), LISTENER_MAGIC_NUMBER, EPOLLIN)
        .expect("epoll add failed");

    Barrier barrier(2);
    std::thread t(
        [&]() {
            auto strm = TcpStream::connect(server_addr).value();
            barrier.wait();
        });

    epoll.poll(events, std::chrono::milliseconds(-1));
    EXPECT_EQ(events.size(), 1);
    EXPECT_TRUE(!!(events[0].events & EPOLLIN));

    auto [c, peer_addr] =
        listener.accept().expect("unable to access connection");
    EXPECT_TRUE(peer_addr.addr().is_loopback());
    EXPECT_EQ(c.peer_addr().value(), peer_addr);
    EXPECT_EQ(c.local_addr().value(), server_addr);

    // no more connections
    auto accept_result = listener.accept();
    EXPECT_TRUE(accept_result.is_error());
    EXPECT_EQ(accept_result.error(), EAGAIN);

    EXPECT_EQ(listener.take_error().value(), 0);

    barrier.wait();
    if (t.joinable()) {
        t.join();
    }
}

TEST(TcpStream, try_clone) {
    auto epoll = Epoll::create().expect("epoll_create failed");
    auto strm = TcpStream::connect(server_addr).value();
    auto strm2 = strm.try_clone().value();

    epoll.add(strm2.as_fd(), nullptr, EPOLLOUT | EPOLLET)
        .expect("epoll add fd failed");

    std::vector<struct epoll_event> events(10);
    epoll.poll(events, std::chrono::milliseconds(-1))
        .expect("epoll_wait failed");
    EXPECT_EQ(events.size(), 1);

    auto& event = events[0];
    EXPECT_EQ(event.data.ptr, nullptr);
    EXPECT_TRUE(!!(event.events & EPOLLOUT));
    EXPECT_TRUE(!!(event.events & EPOLLERR));
    EXPECT_EQ(strm.take_error().value(), ECONNREFUSED);
}

TEST(TcpStream, shutdown) {
    auto strm = TcpStream::connect(server_addr).value();
    auto result = strm.shutdown(SHUT_RDWR);
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error(), ENOTCONN);
}

TEST(TcpStream, send) {
    Barrier barrier(2);

    std::thread([&]() {
        std::vector<struct epoll_event> events(10);
        auto epoll = Epoll::create().expect("epoll_create failed");

        auto listener = TcpListener::bind(server_addr)
                            .expect("bind to 127.0.0.1:8081 failed");

        epoll.add(listener.as_fd(), LISTENER_MAGIC_NUMBER, EPOLLIN)
            .expect("epoll add fd failed");

        barrier.wait();

        while (true) {
            auto eresult = epoll.poll(events, std::chrono::milliseconds(-1));
            if (eresult.is_error()) {
                continue;
            }

            for (auto event : events) {
                if (event.data.ptr == LISTENER_MAGIC_NUMBER) {
                    auto lresult = listener.accept();
                    if (lresult.is_error()) {
                        continue;
                    }

                    auto [strm, addr] = lresult.take_value();
                    const int fd = strm.as_fd();
                    epoll.add(fd, new TcpStream(std::move(strm)), EPOLLIN);

                    continue;
                }

                char buf[1024];
                auto strmp = (TcpStream*)event.data.ptr;
                strmp->recv(buf, sizeof(buf));
            }
        }
    }).detach();

    barrier.wait();

    std::vector<struct epoll_event> events(10);
    auto epoll = Epoll::create().expect("epoll_create failed");

    auto strm = TcpStream::connect(server_addr).value();
    epoll.add(strm.as_fd(), nullptr, EPOLLOUT);

    epoll.poll(events, std::chrono::milliseconds(-1));
    EXPECT_EQ(events.size(), 1);
    EXPECT_TRUE(!!(events[0].events & EPOLLOUT));
    EXPECT_FALSE(events[0].events & ~EPOLLOUT);
    EXPECT_EQ(strm.take_error().value(), 0);

    char buf[1024];
    strm.send(buf, sizeof(buf));
}
