#include "bipolar/net/socket_address.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>

#include <gtest/gtest.h>

using namespace bipolar;

TEST(SocketAddress, GetterSetter) {
    SocketAddress addr1(IPAddress(IPv4Address(127, 0, 0, 1)),
                        hton(static_cast<std::uint16_t>(8086)));
    EXPECT_EQ(addr1.addr(), IPAddress(IPv4Address(127, 0, 0, 1)));
    EXPECT_EQ(addr1.port(), hton(static_cast<std::uint16_t>(8086)));

    SocketAddress addr2(IPAddress(IPv4Address()), 0);
    addr2.addr(IPAddress(IPv4Address(127, 0, 0, 1)));
    EXPECT_EQ(addr2.addr(), IPAddress(IPv4Address(127, 0, 0, 1)));

    SocketAddress addr3(IPAddress(IPv4Address()), 0);
    addr3.port(hton(static_cast<std::uint16_t>(8086)));
    EXPECT_EQ(addr3.port(), hton(static_cast<std::uint16_t>(8086)));
}

TEST(SocketAddress, from_str) {
    auto r1 = SocketAddress::from_str("127.0.0.1:8086").value();
    EXPECT_EQ(r1.addr(), IPAddress(IPv4Address(127, 0, 0, 1)));
    EXPECT_EQ(r1.port(), hton(static_cast<std::uint16_t>(8086)));

    auto r2 = SocketAddress::from_str("[::]:8086").value();
    EXPECT_EQ(r2.addr(), IPAddress(IPv6Address()));
    EXPECT_EQ(r2.port(), hton(static_cast<std::uint16_t>(8086)));

    auto r3 = SocketAddress::from_str("foo:8086");
    EXPECT_FALSE(r3.is_ok());

    auto r4 = SocketAddress::from_str("");
    EXPECT_FALSE(r4.is_ok());
    EXPECT_EQ(r4.error(), SocketAddressFormatError::INVALID_FORMAT);

    auto r5 = SocketAddress::from_str(":8086");
    EXPECT_FALSE(r5.is_ok());
    EXPECT_EQ(r5.error(), SocketAddressFormatError::INVALID_FORMAT);

    auto r6 = SocketAddress::from_str("foo8086");
    EXPECT_FALSE(r6.is_ok());
    EXPECT_EQ(r6.error(), SocketAddressFormatError::INVALID_FORMAT);

    auto r7 = SocketAddress::from_str(" :65536000000000000");
    EXPECT_FALSE(r7.is_ok());
    EXPECT_EQ(r7.error(), SocketAddressFormatError::INVALID_PORT);

    auto r8 = SocketAddress::from_str(" :65537");
    EXPECT_FALSE(r8.is_ok());
    EXPECT_EQ(r8.error(), SocketAddressFormatError::INVALID_PORT);

    auto r9 = SocketAddress::from_str("::1]:8086");
    EXPECT_FALSE(r9.is_ok());
    EXPECT_EQ(r9.error(), SocketAddressFormatError::INVALID_ADDRESS);

    auto r10 = SocketAddress::from_str(":::8086");
    EXPECT_TRUE(r10.is_ok());
    EXPECT_EQ(r10.value().addr(), IPAddress(IPv6Address()));
    EXPECT_EQ(r10.value().port(), hton(static_cast<std::uint16_t>(8086)));
}

TEST(SocketAddress, str) {
    SocketAddress sa(IPAddress{}, 0);
    EXPECT_EQ(sa.str(), "");
}
