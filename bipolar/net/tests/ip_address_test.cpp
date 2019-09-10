#include "bipolar/net/ip_address.hpp"

#include <cstdio>

#include <gtest/gtest.h>

using namespace bipolar;

TEST(IPAddress, Ctor) {
    EXPECT_EQ(IPv4Address(127, 0, 0, 1).str(), "127.0.0.1");

    auto hton = [](int x) {
        return bipolar::hton(static_cast<std::uint16_t>(x));
    };

    IPv6Address v6(hton(0x0011), hton(0x2233), hton(0x4455), hton(0x6677),
                   hton(0x8899), hton(0xaabb), hton(0xccdd), hton(0xeeff));
    EXPECT_EQ(v6.str(), "11:2233:4455:6677:8899:aabb:ccdd:eeff");
}

TEST(IPAddress, Assignment) {

}

TEST(IPAddress, Ordering) {
    {
        IPv4Address addr1(1, 1, 1, 1);
        IPv4Address addr2(1, 1, 1, 2);
        EXPECT_LT(addr1, addr2);
    }

    {
        IPv6Address addr1(0, 0, 0, 0, 1, 1, 1, 1);
        IPv6Address addr2(0, 0, 0, 0, 1, 1, 1, 2);
        EXPECT_LT(addr1, addr2);
    }
}
