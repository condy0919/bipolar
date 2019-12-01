#include "bipolar/net/udp.hpp"

#include <gtest/gtest.h>

#include "bipolar/core/byteorder.hpp"

using namespace bipolar;
using namespace std::string_literals;

TEST(UdpSocket, bind) {
    auto socket = UdpSocket::bind(SocketAddress(IPv4Address(), 0))
                      .expect("couldn't bind to 0.0.0.0");
}

TEST(UdpSocket, try_clone) {
    auto socket = UdpSocket::bind(SocketAddress(IPv4Address(), 0))
                      .expect("couldn't bind to 0.0.0.0");

    auto cloned_socket = socket.try_clone().expect("couldn't clone the socket");

    EXPECT_EQ(socket.local_addr(), cloned_socket.local_addr());
}

TEST(UdpSocket, sendto_and_recvfrom) {
    const auto dst_addr =
        SocketAddress(IPv4Address(), hton(static_cast<std::uint16_t>(8080)));
    auto receiver =
        UdpSocket::bind(dst_addr).expect("couldn't bind to 0.0.0.0:8080");

    auto sender = UdpSocket::bind(SocketAddress(IPv4Address(), 0))
                      .expect("couldn't bind to 0.0.0.0");

    const char send_data[] = "buzz";
    auto send_result = sender.sendto(send_data, 4, dst_addr);
    EXPECT_TRUE(send_result.has_value());
    EXPECT_EQ(send_result.value(), 4);

    char buf[5];
    auto recv_result = receiver.recvfrom(buf, 5);
    EXPECT_TRUE(recv_result.has_value());
    EXPECT_EQ(std::get<0>(recv_result.value()), 4);
    EXPECT_EQ(buf, "buzz"s);

    // dst_addr binds to "0.0.0.0:port" while the received address will be
    // "127.0.0.1:port".
    // So compares port number only.
    EXPECT_EQ(std::get<1>(recv_result.value()).port(),
              sender.local_addr().value().port());
}

TEST(UdpSocket, local_addr) {
    auto socket =
        UdpSocket::bind(SocketAddress(IPv4Address(127, 0, 0, 1),
                                      hton(static_cast<std::uint16_t>(8080))))
            .expect("couldn't bind to 127.0.0.1:8080");

    EXPECT_TRUE(socket.local_addr().has_value());
    EXPECT_EQ(socket.local_addr().value().str(), "127.0.0.1:8080");
}

TEST(UdpSocket, connect_and_peer_addr) {
    auto connector =
        UdpSocket::bind(SocketAddress(IPv4Address(127, 0, 0, 1),
                                      hton(static_cast<std::uint16_t>(8080))))
            .expect("couldn't bind to 127.0.0.1:8080");
    EXPECT_TRUE(connector.peer_addr().has_error());
    EXPECT_EQ(connector.peer_addr().error(), ENOTCONN);

    auto receiver =
        UdpSocket::bind(SocketAddress(IPv4Address(127, 0, 0, 1),
                                      hton(static_cast<std::uint16_t>(8081))))
            .expect("coundn't bind to 127.0.0.1:8081");

    auto connect_result = connector.connect(SocketAddress(
        IPv4Address(127, 0, 0, 1), hton(static_cast<std::uint16_t>(8081))));
    EXPECT_TRUE(connect_result.has_value());

    auto peer_addr_result = connector.peer_addr();
    EXPECT_TRUE(peer_addr_result.has_value());
    EXPECT_EQ(peer_addr_result.value().str(), "127.0.0.1:8081");
}

TEST(UdpSocket, as_fd) {
    auto socket = UdpSocket(-1);
    EXPECT_EQ(socket.as_fd(), -1);
}
