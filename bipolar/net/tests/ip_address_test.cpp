#include "bipolar/net/ip_address.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>

#include <gtest/gtest.h>

using namespace bipolar;

TEST(IPAddress, Assignment) {
    auto addr = IPAddress(IPv4Address(0, 0, 0, 0));
    EXPECT_EQ(addr.str(), "0.0.0.0");

    addr = IPv6Address(0, 0, 0, 0, 0, 0, 0, 0);
    EXPECT_EQ(addr.str(), "::");
}

TEST(IPAddress, Ordering) {
    {
        IPv4Address addr1(1, 1, 1, 1);
        IPv4Address addr2(1, 1, 1, 2);
        EXPECT_LT(addr1, addr2);
    }

    {
        IPv4Address addr1(0, 0, 0, 0);
        IPv4Address addr2(0, 0, 0, 0);
        EXPECT_EQ(addr1, addr2);
    }

    {
        IPv6Address addr1(0, 0, 0, 0, 1, 1, 1, 1);
        IPv6Address addr2(0, 0, 0, 0, 1, 1, 1, 2);
        EXPECT_LT(addr1, addr2);
    }

    {
        IPv6Address addr1(0, 0, 0, 0, 0, 0, 0, 0);
        IPv6Address addr2(0, 0, 0, 0, 0, 0, 0, 0);
        EXPECT_EQ(addr1, addr2);
    }
}

struct AddressData {
    std::string addr;
    std::vector<std::uint8_t> bytes;
    bool valid;
    bool is_ipv4;
    bool is_ipv6;
    bool is_unspecified;
    bool is_loopback;
};

struct IPAddressTest : testing::TestWithParam<AddressData> {
    void expect() {
        auto param = GetParam();

        auto result = IPAddress::from_str(param.addr);
        if (!param.valid) {
            EXPECT_FALSE(result.is_ok());
            return;
        }

        auto addr = result.value();
        EXPECT_EQ(addr.is_ipv4(), param.is_ipv4);
        EXPECT_EQ(addr.is_ipv6(), param.is_ipv6);
        EXPECT_EQ(addr.is_unspecified(), param.is_unspecified);
        EXPECT_EQ(addr.is_loopback(), param.is_loopback);

        if (addr.is_ipv4()) {
            const auto& ipv4 = addr.as_ipv4();
            const auto octets = ipv4.octets();
            EXPECT_TRUE(std::equal(param.bytes.begin(), param.bytes.end(),
                                   octets.begin()));

            {
                const auto& ipv6 = ipv4.to_ipv6_mapped();
                EXPECT_EQ(ipv6.to_ipv4().value(), ipv4);
            }

            {
                const auto& ipv6 = ipv4.to_ipv6_compatible();
                EXPECT_EQ(ipv6.to_ipv4().value(), ipv4);
            }
        } else {
            const auto& ipv6 = addr.as_ipv6();
            const auto octets = ipv6.octets();
            EXPECT_TRUE(std::equal(param.bytes.begin(), param.bytes.end(),
                                   octets.begin()));
        }

        EXPECT_EQ(addr.str(), param.addr);
    }
};

static const std::vector<AddressData> address_provider = {
    {"127.0.0.1", {127, 0, 0, 1}, true, true, false, false, true},
    {"0.0.0.0", {0, 0, 0, 0}, true, true, false, true, false},
    {"8.8.8.8", {8, 8, 8, 8}, true, true, false, false, false},
    {"1.1.1", {}, false, false, false, false, false},
    {"::",
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     true,
     false,
     true,
     true,
     false},
    {"::1",
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
     true,
     false,
     true,
     false,
     true},
    {"11:2233:4455:6677:8899:aabb:ccdd:eeff",
     {0x0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb,
      0xcc, 0xdd, 0xee, 0xff},
     true,
     false,
     true,
     false,
     false},
    {"foo", {}, false, false, false, false, false},
};

INSTANTIATE_TEST_CASE_P(IPAddress, IPAddressTest,
                        ::testing::ValuesIn(address_provider));

TEST_P(IPAddressTest, All) {
    expect();
}

TEST(IPv4Address, to_long) {
    IPv4Address v4(hton(static_cast<std::uint32_t>(0x11223344)));

    EXPECT_EQ(v4.str(), "17.34.51.68");
    EXPECT_EQ(v4.to_long(), hton(static_cast<std::uint32_t>(0x11223344)));
}

TEST(IPv6Address, to_ipv4) {
    auto addr1 =
        IPv6Address::from_str("11:2233:4455:6677:8899:aabb:ccdd:eeff").value();
    EXPECT_FALSE(bool(addr1.to_ipv4()));

    auto addr2 = IPv6Address::from_str("::1.2.4.8").value();
    EXPECT_TRUE(bool(addr2.to_ipv4()));
    EXPECT_EQ(addr2.to_ipv4().value().str(), "1.2.4.8");

    auto addr3 = IPv6Address::from_str("::ffff:1.2.4.8").value();
    EXPECT_TRUE(bool(addr3.to_ipv4()));
    EXPECT_EQ(addr3.to_ipv4().value().str(), "1.2.4.8");
}
