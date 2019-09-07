#include "bipolar/core/byteorder.hpp"

#include <gtest/gtest.h>

using namespace bipolar;

#define BYTEORDER_TESTCASES(G, f)                                              \
    G(f, std::uint8_t(0x12), std::uint8_t(0x12))                               \
    G(f, std::uint16_t(0x1234), std::uint16_t(0x3412))                         \
    G(f, std::uint32_t(0x12345678), std::uint32_t(0x78563412))                 \
    G(f, std::uint64_t(0x1234567812345678), std::uint64_t(0x7856341278563412))

#define G(f, x, x_rev)                                                         \
    if constexpr (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) {                    \
        EXPECT_EQ(f(x), x);                                                    \
    } else {                                                                   \
        EXPECT_EQ(f(x), x_rev);                                                \
    }

TEST(ByteOrder, hton) {
    BYTEORDER_TESTCASES(G, hton)
}

TEST(ByteOrder, ntoh) {
    BYTEORDER_TESTCASES(G, ntoh)
}
