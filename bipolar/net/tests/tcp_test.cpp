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

#include <gtest/gtest.h>

using namespace bipolar;
using namespace std::string_literals;

inline constexpr SocketAddress
    server_addr(IPv4Address(127, 0, 0, 1),
                hton(static_cast<std::uint16_t>(8081)));

const auto LISTENER_MAGIC_NUMBER =
    reinterpret_cast<void*>(static_cast<std::uintptr_t>(0x50043));

const auto EPOLL_SUSPEND_MAGIC_NUMBER =
    reinterpret_cast<void*>(static_cast<std::uintptr_t>(0x50044));

struct StreamSession {
    TcpStream strm;
};

static std::tuple<std::thread, int>
test_client_and_server(Function<void()> accept_cb, Function<void()> send_cb,
                       Function<void()> recv_cb) {
    int efd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (efd == -1) {
        exit(-2);
    }

    std::thread t([efd, accept_cb = std::move(accept_cb),
                   send_cb = std::move(send_cb),
                   recv_cb = std::move(recv_cb)]() {
        auto epoll = Epoll::create().expect("epoll_create failed");

        epoll.add(efd, EPOLL_SUSPEND_MAGIC_NUMBER, EPOLLIN)
            .expect("epoll add eventfd failed");

        auto listener = TcpListener::bind(server_addr)
                            .expect("failed to bind 127.0.0.1:8081");

        epoll.add(listener.as_fd(), LISTENER_MAGIC_NUMBER, EPOLLIN)
            .expect("epoll add listener fd failed");

        auto strm = TcpStream::connect(server_addr)
                        .expect("connect to 127.0.0.1:8081 failed");

        const int sfd = strm.as_fd();
        auto ssp = new StreamSession{
            std::move(strm),
        };
        epoll.add(sfd, ssp, EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET)
            .expect("epoll add fd failed");

        std::vector<struct epoll_event> events(10);
        while (true) {
            if (auto result = epoll.poll(events, std::chrono::milliseconds(-1));
                result.is_error()) {
                continue;
            }

            for (auto& event : events) {
                if (event.data.ptr == EPOLL_SUSPEND_MAGIC_NUMBER) {
                    return;
                }

                if (event.data.ptr == LISTENER_MAGIC_NUMBER) {
                    // prevent for accept starvation
                    for (std::size_t i = 0; i < 10; ++i) {
                        auto result = listener.accept();
                        if (result.is_error()) {
                            if (result.error() == EAGAIN) {
                                break;
                            } else {
                                return;
                            }
                        }

                        accept_cb();

                        auto [new_stream, new_addr] = result.take_value();
                        auto new_addr_str = new_addr.str();
                        const int fd = new_stream.as_fd();
                        epoll
                            .add(fd,
                                 new StreamSession{
                                     std::move(new_stream),
                                 },
                                 EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET)
                            .expect("epoll add fd failed");
                    }
                } else {
                    auto ssp = static_cast<StreamSession*>(event.data.ptr);
                    if ((event.events & EPOLLOUT) ||
                        (event.events & EPOLLHUP)) {
                        auto r = ssp->strm.send("test", 4, MSG_NOSIGNAL);
                        if (r.is_ok()) {
                            send_cb();
                        } else if (r.is_error() && r.error() != EAGAIN) {
                            ssp->strm.close();
                        }
                    }
                    if (event.events & ~EPOLLOUT) {
                        char buf[256];
                        auto r = ssp->strm.recv(buf, sizeof(buf), MSG_NOSIGNAL);
                        if (r.is_ok()) {
                            recv_cb();
                        } else if (r.is_error() && r.error() != EAGAIN) {
                            // write hand may close strm early.
                            // close may encounter EBADF errno now, but it's ok.
                            // there is no new fd allocated in this simple test.
                            ssp->strm.close();
                        }
                    }
                }
            }
        }
    });

    return std::make_tuple(std::move(t), efd);
}

TEST(TcpListener, accept_connect) {
    std::size_t cnt = 0;
    auto [thread, efd] = test_client_and_server(
        /* accept_cb = */
        Function<void()>([&cnt]() { ++cnt; }),
        /* send_cb = */
        Function<void()>([]() {}),
        /* recv_cb = */
        Function<void()>([]() {}));

    sleep(1);

    ::eventfd_write(efd, 1);
    if (thread.joinable()) {
        thread.join();
    }

    EXPECT_EQ(cnt, 1);
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
    EXPECT_EQ(event.events & EPOLLOUT, EPOLLOUT);
    EXPECT_EQ(strm.take_error().value(), ECONNREFUSED);
}

TEST(TcpStream, send) {
    std::size_t cnt = 0;
    auto [thread, efd] = test_client_and_server(
        /* accept_cb = */
        Function<void()>([]() {}),
        /* send_cb = */
        Function<void()>([&cnt]() {
            ++cnt;
            pthread_exit(nullptr);
        }),
        /* recv_cb = */
        Function<void()>([]() {}));

    sleep(1);

    ::eventfd_write(efd, 1);
    if (thread.joinable()) {
        thread.join();
    }

    EXPECT_EQ(cnt, 1);
}
