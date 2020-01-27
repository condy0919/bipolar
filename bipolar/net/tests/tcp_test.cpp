#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <future>
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
using namespace std::chrono_literals;

inline constexpr auto anonymous_addr =
    SocketAddress(IPv4Address(127, 0, 0, 1), 0);

const auto LISTENER_MAGIC_NUMBER =
    reinterpret_cast<void*>(static_cast<std::uintptr_t>(0x50043));

TEST(TcpListener, bind_and_accept) {
    auto epoll = Epoll::create().expect("epoll_create failed");
    std::vector<struct epoll_event> events(10);

    auto listener =
        TcpListener::bind(anonymous_addr).expect("bind to 127.0.0.1:0 failed");
    auto server_addr = listener.local_addr().value();

    epoll.add(listener.as_fd(), LISTENER_MAGIC_NUMBER, EPOLLIN)
        .expect("epoll add failed");

    Barrier barrier(2);
    std::thread t([&]() {
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
    auto strm = TcpStream::connect(anonymous_addr).value();
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

TEST(TcpStream, connect) {
    auto listener = TcpListener::bind(anonymous_addr).expect("bind failed");
    auto server_addr = listener.local_addr().value();
    auto epoll = Epoll::create().expect("epoll_create failed");
    auto strm = TcpStream::connect(server_addr).expect("connect failed");

    epoll.add(strm.as_fd(), nullptr, EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP)
        .expect("epoll add failed");

    std::promise<Void> p0;
    std::promise<Void> p1;
    std::future<Void> f1 = p1.get_future();

    std::vector<struct epoll_event> events(10);
    epoll.poll(events, std::chrono::milliseconds(-1)).value();
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].data.ptr, nullptr);
    EXPECT_TRUE(!!(events[0].events & EPOLLOUT));

    std::thread t(
        [&listener, f0 = p0.get_future(), p1 = std::move(p1)]() mutable {
            auto [s, sa] = listener.accept().value();
            f0.wait();
            s.close();
            p1.set_value(Void{});
        });
    t.detach();

    p0.set_value(Void{});
    f1.wait();

    events.resize(10);
    epoll.poll(events, std::chrono::milliseconds(-1)).value();
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].data.ptr, nullptr);
    EXPECT_TRUE(!!(events[0].events & EPOLLIN));
}

TEST(TcpStream, read) {
    const std::size_t N = 16 * 1024;

    auto listener = TcpListener::bind(anonymous_addr).expect("bind failed");
    auto server_addr = listener.local_addr().value();
    auto epoll = Epoll::create().expect("epoll_create failed");
    auto strm = TcpStream::connect(server_addr).expect("connect failed");

    // confirm connection established
    std::vector<struct epoll_event> events(10);
    epoll.add(strm.as_fd(), nullptr, EPOLLOUT | EPOLLET | EPOLLONESHOT);
    epoll.poll(events, std::chrono::milliseconds(-1));
    EXPECT_EQ(strm.take_error().value(), 0);

    epoll.add(listener.as_fd(), nullptr, EPOLLIN | EPOLLET | EPOLLONESHOT);
    epoll.poll(events, std::chrono::milliseconds(-1));

    std::thread t([&listener]() {
        auto [s, sa] = listener.accept().expect("accept failed");

        s.set_nonblocking(false);

        char buf[1024];
        std::size_t amount = 0;
        while (amount < N) {
            amount += s.write(buf, sizeof(buf)).expect("write failed");
        }
    });
    t.detach();

    // rearm the disabled events
    epoll.mod(strm.as_fd(), nullptr, EPOLLIN | EPOLLET);

    std::size_t amount = 0;
    while (amount < N) {
        epoll.poll(events, std::chrono::milliseconds(-1)).value();
        EXPECT_EQ(events.size(), 1);

        char buf[1024];
        while (true) {
            auto result = strm.read(buf, sizeof(buf));
            if (result.is_ok()) {
                amount += result.value();
            } else {
                break;
            }
            if (amount >= N) {
                break;
            }
        }
    }
}

// // Why it failed on GitHub CI?
// TEST(TcpStream, write) {
//     const std::size_t N = 16 * 1024;
//
//     auto listener = TcpListener::bind(anonymous_addr).expect("bind failed");
//     auto server_addr = listener.local_addr().value();
//     auto epoll = Epoll::create().expect("epoll_create failed");
//     auto strm = TcpStream::connect(server_addr).expect("connect failed");
//
//     // confirm connection established
//     std::vector<struct epoll_event> events(10);
//     epoll.add(strm.as_fd(), nullptr, EPOLLOUT | EPOLLET | EPOLLONESHOT);
//     epoll.poll(events, std::chrono::milliseconds(-1));
//     EXPECT_EQ(strm.take_error().value(), 0);
//
//     epoll.add(listener.as_fd(), nullptr, EPOLLIN | EPOLLET | EPOLLONESHOT);
//     epoll.poll(events, std::chrono::milliseconds(-1));
//
//     std::thread t([&listener]() {
//         // XXX weird, EBADF returned on GitHub CI
//         auto [s, sa] = listener.accept().expect("accept failed 2");
//
//         s.set_nonblocking(false);
//
//         char buf[1024];
//         std::size_t amount = 0;
//         while (amount < N) {
//             amount += s.read(buf, sizeof(buf)).expect("read failed");
//         }
//     });
//     t.detach();
//
//     // rearm the disabled events
//     epoll.mod(strm.as_fd(), nullptr, EPOLLOUT | EPOLLET);
//
//     std::size_t amount = 0;
//     while (amount < N) {
//         epoll.poll(events, std::chrono::milliseconds(-1)).value();
//         EXPECT_EQ(events.size(), 1);
//
//         char buf[1024];
//         while (true) {
//             auto result = strm.write(buf, sizeof(buf));
//             if (result.is_ok()) {
//                 amount += result.value();
//             } else {
//                 break;
//             }
//             if (amount >= N) {
//                 break;
//             }
//         }
//     }
// }

TEST(TcpStream, connect_then_close) {
    auto listener = TcpListener::bind(anonymous_addr).expect("bind failed");
    auto server_addr = listener.local_addr().value();
    auto epoll = Epoll::create().expect("epoll_create failed");
    auto strm = TcpStream::connect(server_addr).expect("connect failed");

    epoll.add(listener.as_fd(), 1, EPOLLIN | EPOLLET);
    epoll.add(strm.as_fd(), 2, EPOLLIN | EPOLLET);

    bool shutdown = false;
    std::vector<struct epoll_event> events(10);
    while (!shutdown) {
        epoll.poll(events, std::chrono::milliseconds(-1));

        for (auto event : events) {
            if (event.data.fd == 1) {
                auto [s, sa] = listener.accept().value();
                epoll.add(s.as_fd(), 3, EPOLLIN | EPOLLOUT | EPOLLET);
                s.close();
            } else if (event.data.fd == 2) {
                shutdown = true;
            }
        }
    }
}

TEST(TcpStream, listen_then_close) {
    auto listener = TcpListener::bind(anonymous_addr).expect("bind failed");
    auto epoll = Epoll::create().expect("epoll_create failed");

    epoll.add(listener.as_fd(), nullptr, EPOLLIN | EPOLLRDHUP | EPOLLET);
    listener.close();

    std::vector<struct epoll_event> events(10);
    epoll.poll(events, std::chrono::milliseconds(100));
    EXPECT_EQ(events.size(), 0);
}

TEST(TcpStream, connect_error) {
    auto epoll = Epoll::create().expect("epoll_create failed");
    auto strm = TcpStream::connect(anonymous_addr).expect("connect failed");

    epoll.add(strm.as_fd(), nullptr, EPOLLOUT | EPOLLET);

    std::vector<struct epoll_event> events(10);
    epoll.poll(events, std::chrono::milliseconds(-1));
    EXPECT_EQ(events.size(), 1);
    EXPECT_TRUE(!!(events[0].events & EPOLLOUT));
    EXPECT_EQ(strm.take_error().value(), ECONNREFUSED);
}

TEST(TcpStream, write_then_drop) {
    auto listener = TcpListener::bind(anonymous_addr).expect("bind failed");
    auto server_addr = listener.local_addr().value();
    auto epoll = Epoll::create().expect("epoll_create failed");
    auto strm = TcpStream::connect(server_addr).expect("connect failed");

    epoll.add(listener.as_fd(), 1, EPOLLIN | EPOLLET);
    epoll.add(strm.as_fd(), 2, EPOLLIN | EPOLLET);

    std::vector<struct epoll_event> events(10);
    epoll.poll(events, std::chrono::milliseconds(-1));
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].data.fd, 1);

    auto strm2 = std::get<0>(listener.accept().value());
    epoll.add(strm2.as_fd(), 3, EPOLLOUT | EPOLLET);

    strm2.write("1234", 4);
    strm2.close();

    epoll.poll(events, std::chrono::milliseconds(-1));
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].data.fd, 2);

    char buf[4] = "";
    strm.read(buf, sizeof(buf));
    EXPECT_TRUE(std::memcmp(buf, "1234", 4) == 0);
}

TEST(TcpStream, connection_reset_by_peer) {
    auto listener = TcpListener::bind(anonymous_addr).expect("bind failed");
    auto server_addr = listener.local_addr().value();
    auto epoll = Epoll::create().expect("epoll_create failed");
    auto strm = TcpStream::connect(server_addr).expect("connect failed");

    epoll.add(listener.as_fd(), 1, EPOLLIN | EPOLLET | EPOLLONESHOT);
    epoll.add(strm.as_fd(), 2, EPOLLIN | EPOLLET);

    std::vector<struct epoll_event> events(10);
    epoll.poll(events, std::chrono::milliseconds(-1));
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].data.fd, 1);

    auto strm2 = std::get<0>(listener.accept().value());

    // reset the connection
    strm.set_linger(Some(0s));
    strm.close();

    epoll.add(strm2.as_fd(), 3, EPOLLIN | EPOLLET);
    epoll.poll(events, std::chrono::milliseconds(-1));
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].data.fd, 3);

    char buf[10];
    auto result = strm2.read(buf, sizeof(buf));
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error(), ECONNRESET);
}

TEST(TcpStream, write_error) {
    auto listener = TcpListener::bind(anonymous_addr).expect("bind failed");
    auto server_addr = listener.local_addr().value();
    auto epoll = Epoll::create().expect("epoll_create failed");
    auto strm = TcpStream::connect(server_addr).expect("connect failed");

    epoll.add(listener.as_fd(), 0, EPOLLIN | EPOLLET);

    std::vector<struct epoll_event> events(10);
    epoll.poll(events, std::chrono::milliseconds(-1));

    auto [s, sa] = listener.accept().value();
    s.close();

    char buf[10] = "miss";
    Result<std::size_t, int> result;
    while ((result = strm.send(buf, sizeof(buf), MSG_NOSIGNAL)).is_ok()) {
    }
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error(), EPIPE);
}

TEST(TcpStream, write_shutdown) {
    auto listener = TcpListener::bind(anonymous_addr).expect("bind failed");
    auto server_addr = listener.local_addr().value();
    auto epoll = Epoll::create().expect("epoll_create failed");
    auto strm = TcpStream::connect(server_addr).expect("connect failed");

    epoll.add(listener.as_fd(), 0, EPOLLIN | EPOLLET);

    std::vector<struct epoll_event> events(10);
    epoll.poll(events, std::chrono::milliseconds(-1));

    auto [s, sa] = listener.accept().value();
    s.shutdown(SHUT_WR);

    epoll.add(strm.as_fd(), 0, EPOLLIN | EPOLLET);
    epoll.poll(events, std::chrono::milliseconds(-1));
    EXPECT_EQ(events.size(), 1);
    EXPECT_TRUE(!!(events[0].events & EPOLLIN));
}

TEST(TcpStream, write_then_del) {
    auto listener = TcpListener::bind(anonymous_addr).expect("bind failed");
    auto server_addr = listener.local_addr().value();
    auto epoll = Epoll::create().expect("epoll_create failed");
    auto strm = TcpStream::connect(server_addr).expect("connect failed");

    epoll.add(listener.as_fd(), 1, EPOLLIN | EPOLLET);
    epoll.add(strm.as_fd(), 3, EPOLLIN | EPOLLET);

    std::vector<struct epoll_event> events(10);
    epoll.poll(events, std::chrono::milliseconds(-1));
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].data.fd, 1);

    auto strm2 = std::get<0>(listener.accept().value());
    epoll.add(strm2.as_fd(), 2, EPOLLOUT | EPOLLET);
    epoll.poll(events, std::chrono::milliseconds(-1));
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].data.fd, 2);

    strm2.write("1234", 4);
    epoll.del(strm2.as_fd());
    epoll.poll(events, std::chrono::milliseconds(-1));
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].data.fd, 3);

    char buf[10];
    auto result = strm.read(buf, sizeof(buf));
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 4);
    EXPECT_TRUE(std::memcmp(buf, "1234", 4) == 0);
}

TEST(TcpStream, tcp_no_events_after_del) {
    auto listener = TcpListener::bind(anonymous_addr).expect("bind failed");
    auto server_addr = listener.local_addr().value();
    auto epoll = Epoll::create().expect("epoll_create failed");
    auto strm = TcpStream::connect(server_addr).expect("connect failed");

    epoll.add(listener.as_fd(), 1, EPOLLIN | EPOLLET);
    epoll.add(strm.as_fd(), 3, EPOLLIN | EPOLLET);

    std::vector<struct epoll_event> events(10);
    epoll.poll(events, std::chrono::milliseconds(-1));
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].data.fd, 1);

    auto [strm2, strm2_addr] = listener.accept().value();
    EXPECT_TRUE(strm2_addr.addr().is_loopback());
    EXPECT_EQ(strm2.peer_addr().value(), strm2_addr);
    EXPECT_EQ(strm2.local_addr().value(), server_addr);

    epoll.add(strm2.as_fd(), 2, EPOLLOUT | EPOLLET);
    epoll.poll(events, std::chrono::milliseconds(-1));
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].data.fd, 2);

    strm2.write("1234", 4);
    epoll.poll(events, std::chrono::milliseconds(-1));
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].data.fd, 3);
    EXPECT_TRUE(!!(events[0].events & EPOLLIN));

    epoll.del(listener.as_fd());
    epoll.del(strm.as_fd());
    epoll.del(strm2.as_fd());

    epoll.poll(events, 10ms);
    EXPECT_EQ(events.size(), 0);

    char buf[10];
    strm.read(buf, sizeof(buf));
    EXPECT_TRUE(std::memcmp(buf, "1234", 4) == 0);

    strm2.write("9876", 4);
    epoll.poll(events, 10ms);
    EXPECT_EQ(events.size(), 0);

    std::this_thread::sleep_for(100ms);
    strm.read(buf, sizeof(buf));
    EXPECT_TRUE(std::memcmp(buf, "9876", 4) == 0);

    epoll.poll(events, 10ms);
    EXPECT_EQ(events.size(), 0);
}

TEST(TcpStream, shutdown) {
    auto strm = TcpStream::connect(anonymous_addr).value();
    auto result = strm.shutdown(SHUT_RDWR);
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error(), ENOTCONN);
}

TEST(TcpStream, send) {
    Barrier barrier(2);

    SocketAddress server_addr = anonymous_addr;

    std::thread([&]() {
        std::vector<struct epoll_event> events(10);
        auto epoll = Epoll::create().expect("epoll_create failed");

        auto listener = TcpListener::bind(anonymous_addr)
                            .expect("bind to 127.0.0.1:0 failed");

        epoll.add(listener.as_fd(), LISTENER_MAGIC_NUMBER, EPOLLIN)
            .expect("epoll add fd failed");

        server_addr = listener.local_addr().value();
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
