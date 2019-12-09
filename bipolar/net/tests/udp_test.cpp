#include "bipolar/net/udp.hpp"

#include <gtest/gtest.h>

#include "bipolar/core/byteorder.hpp"

using namespace bipolar;
using namespace std::string_literals;

namespace {
template <typename F>
void connected_test(F&& f) {
    auto sender =
        UdpSocket::bind(SocketAddress(IPv4Address(127, 0, 0, 1),
                                      hton(static_cast<std::uint16_t>(8080))))
            .expect("couldn't bind to 127.0.0.1:8080");

    auto receiver =
        UdpSocket::bind(SocketAddress(IPv4Address(127, 0, 0, 1),
                                      hton(static_cast<std::uint16_t>(8081))))
            .expect("couldn't bind to 127.0.0.1:8081");

    sender
        .connect(SocketAddress(IPv4Address(127, 0, 0, 1),
                               hton(static_cast<std::uint16_t>(8081))))
        .expect("couldn't connect to 127.0.0.1:8081");

    f(sender, receiver);
}

template <typename F>
void connected_test_v6(F&& f) {
    auto sender =
        UdpSocket::bind(SocketAddress(IPv6Address::from_str("::1").value(),
                                      hton(static_cast<std::uint16_t>(8080))))
            .expect("couldn't bind to ::1");

    auto receiver =
        UdpSocket::bind(SocketAddress(IPv6Address::from_str("::1").value(),
                                      hton(static_cast<std::uint16_t>(8081))))
            .expect("couldn't bind to ::1");

    sender
        .connect(SocketAddress(IPv6Address::from_str("::1").value(),
                               hton(static_cast<std::uint16_t>(8081))))
        .expect("couldn't connect to 127.0.0.1:8081");

    f(sender, receiver);
}

} // namespace

TEST(UdpSocket, bind) {
    connected_test([](UdpSocket&, UdpSocket&) {});
}

TEST(UdpSocket, bind_failed) {
    const auto addr = SocketAddress(IPv4Address(1, 1, 1, 1), 0);
    auto socket = UdpSocket::bind(addr);
    EXPECT_TRUE(socket.is_error());
    EXPECT_EQ(socket.error(), EADDRNOTAVAIL);
}

TEST(UdpSocket, try_clone) {
    connected_test([](UdpSocket& sender, UdpSocket&) {
        auto cloned_sender =
            sender.try_clone().expect("couldn't clone the socet");
        EXPECT_EQ(sender.local_addr(), cloned_sender.local_addr());
    });
}

TEST(UdpSocket, try_clone_failed) {
    auto socket = UdpSocket(-1);
    auto cloned_socket = socket.try_clone();
    EXPECT_TRUE(cloned_socket.is_error());
    EXPECT_EQ(cloned_socket.error(), EBADF);
}

TEST(UdpSocket, connect) {
    connected_test([](UdpSocket&, UdpSocket&) {});
}

TEST(UdpSocket, connect_failed) {
    connected_test([](UdpSocket& sender, UdpSocket& receiver) {
        auto result = sender.connect(SocketAddress(IPv4Address(1, 1, 1, 1), 0));
        EXPECT_TRUE(result.is_error());
        EXPECT_EQ(result.error(), EINVAL);
    });
}

TEST(UdpSocket, send_and_recv) {
    const char send_buf[] = "buzz";
    char recv_buf[10];

    connected_test([&](UdpSocket& sender, UdpSocket& receiver) {
        const auto send_result = sender.send(send_buf, 4);
        EXPECT_TRUE(send_result.is_ok());
        EXPECT_EQ(send_result.value(), 4);

        const auto recv_result = receiver.recv(recv_buf, 10);
        EXPECT_TRUE(recv_result.is_ok());
        EXPECT_EQ(recv_result.value(), 4);
    });
}

TEST(UdpSocket, write_and_read) {
    const char write_buf[] = "fizz";
    char read_buf[10];

    connected_test([&](UdpSocket& sender, UdpSocket& receiver) {
        const auto write_result = sender.write(write_buf, 4);
        EXPECT_TRUE(write_result.is_ok());
        EXPECT_EQ(write_result.value(), 4);

        const auto read_result = receiver.read(read_buf, 10);
        EXPECT_TRUE(read_result.is_ok());
        EXPECT_EQ(read_result.value(), 4);
    });
}

TEST(UdpSocket, sendto_and_recvfrom) {
    const char send_buf[] = "buzz";
    char recv_buf[10] = "";

    connected_test([&](UdpSocket& sender, UdpSocket& receiver) {
        const auto send_result =
            sender.sendto(send_buf, 4, receiver.local_addr().value());
        EXPECT_TRUE(send_result.is_ok());
        EXPECT_EQ(send_result.value(), 4);

        const auto recv_result = receiver.recvfrom(recv_buf, 10);
        EXPECT_TRUE(recv_result.is_ok());
        EXPECT_EQ(std::get<0>(recv_result.value()), 4);
        EXPECT_EQ(recv_buf, "buzz"s);

        EXPECT_EQ(std::get<1>(recv_result.value()),
                  sender.local_addr().value());
    });
}

TEST(UdpSocket, writev_and_readv) {
    connected_test([](UdpSocket& sender, UdpSocket& receiver) {
        char send_buf[] = "bizz";
        char recv_buf[10] = "";
        struct iovec iov = {
            .iov_base = send_buf,
            .iov_len = 4,
        };

        const auto send_result = sender.writev(&iov, 1);
        EXPECT_TRUE(send_result.is_ok());
        EXPECT_EQ(send_result.value(), 4);

        iov.iov_base = recv_buf;
        iov.iov_len = 10;
        const auto recv_result = receiver.readv(&iov, 1);
        EXPECT_TRUE(recv_result.is_ok());
        EXPECT_EQ(recv_result.value(), 4);
        EXPECT_EQ(recv_buf, "bizz"s);
    });
}

TEST(UdpSocket, sendmsg_and_recvmmsg) {
    auto socket =
        UdpSocket::bind(SocketAddress(IPv4Address(127, 0, 0, 1),
                                      hton(static_cast<std::uint16_t>(8082))))
            .expect("couldn't bind to 127.0.0.1:8082");

    auto addr = socket.local_addr().value().to_sockaddr();

    char send_buf[] = "bizz";
    char recv_buf[10] = "";
    struct iovec iov = {
        .iov_base = send_buf,
        .iov_len = 4,
    };

    struct msghdr msg = {
        .msg_name = &addr,
        .msg_namelen = sizeof(struct sockaddr_in),
        .msg_iov = &iov,
        .msg_iovlen = 1,
    };

    connected_test([&](UdpSocket& sender, UdpSocket& receiver) {
        const auto send_result = sender.sendmsg(&msg);
        EXPECT_TRUE(send_result.is_ok());
        EXPECT_EQ(send_result.value(), 4);

        EXPECT_EQ(sender.peer_addr().value(), receiver.local_addr().value());

        iov.iov_base = recv_buf;
        iov.iov_len = 10;

        std::memset(&msg, 0, sizeof(msg));
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        const auto recv_result = socket.recvmsg(&msg);
        EXPECT_TRUE(recv_result.is_ok());
        EXPECT_EQ(recv_buf, "bizz"s);
    });
}

TEST(UdpSocket, sendmmsg_and_recvmmsg) {
    auto socket =
        UdpSocket::bind(SocketAddress(IPv4Address(127, 0, 0, 1),
                                      hton(static_cast<std::uint16_t>(8082))))
            .expect("couldn't bind to 127.0.0.1:8082");

    auto addr = socket.local_addr().value().to_sockaddr();

    char send_buf[] = "fizz";
    char recv_buf[10] = "";
    struct iovec iov = {
        .iov_base = send_buf,
        .iov_len = 4,
    };

    struct mmsghdr mmsg = {
        .msg_hdr =
            {
                .msg_name = &addr,
                .msg_namelen = sizeof(struct sockaddr_in),
                .msg_iov = &iov,
                .msg_iovlen = 1,
            },
        .msg_len = 0,
    };

    connected_test([&](UdpSocket& sender, UdpSocket& receiver) {
        const auto send_result = sender.sendmmsg(&mmsg, 1);
        EXPECT_TRUE(send_result.is_ok());
        EXPECT_EQ(send_result.value(), 1);
        EXPECT_EQ(mmsg.msg_len, 4);

        EXPECT_EQ(sender.peer_addr().value(), receiver.local_addr().value());

        iov.iov_base = recv_buf;
        iov.iov_len = 10;

        std::memset(&mmsg, 0, sizeof(mmsg));
        mmsg.msg_hdr.msg_iov = &iov;
        mmsg.msg_hdr.msg_iovlen = 1;

        const auto recv_result = socket.recvmmsg(&mmsg, 1);
        EXPECT_TRUE(recv_result.is_ok());
        EXPECT_EQ(recv_result.value(), 1);
        EXPECT_EQ(mmsg.msg_len, 4);
        EXPECT_EQ(recv_buf, "fizz"s);
    });
}

TEST(UdpSocket, local_addr) {
    connected_test([](UdpSocket& sender, UdpSocket&) {
        EXPECT_TRUE(sender.local_addr().is_ok());
        EXPECT_EQ(sender.local_addr().value().str(), "127.0.0.1:8080");
    });
}

TEST(UdpSocket, peer_addr) {
    connected_test([](UdpSocket& sender, UdpSocket& receiver) {
        EXPECT_TRUE(sender.peer_addr().is_ok());
        EXPECT_EQ(sender.peer_addr().value(), receiver.local_addr().value());
    });

    // // ipv6 not supported in ci
    // connected_test_v6([](UdpSocket& sender, UdpSocket& receiver) {
    //     EXPECT_TRUE(sender.peer_addr().is_ok());
    //     EXPECT_EQ(sender.peer_addr().value(), receiver.local_addr().value());
    // });
}

TEST(UdpSocket, as_fd) {
    auto socket = UdpSocket(-1);
    EXPECT_EQ(socket.as_fd(), -1);
}

TEST(UdpSocket, take_error) {
    connected_test([](UdpSocket& sender, UdpSocket& receiver) {
        const auto err = sender.take_error();
        EXPECT_TRUE(err.is_ok());
        EXPECT_EQ(err.value(), 0);
    });
}
