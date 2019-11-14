//! Byteorder helpful methods
//!
//! It converts values between host and network byteorder
//!
//! # Examples
//!
//! ```
//! const std::uint16_t b = hton(0x1122);
//! assert(ntoh(b) == 0x1122);
//! ```

#ifndef BIPOLAR_CORE_BYTEORDER_HPP_
#define BIPOLAR_CORE_BYTEORDER_HPP_

#include <cstdint>

namespace bipolar {
namespace detail {
inline constexpr bool is_big_endian = (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__);
}

/// Converts from host byteorder to network byteorder
constexpr inline std::uint8_t hton(std::uint8_t v) noexcept {
    return v;
}

constexpr inline std::uint16_t hton(std::uint16_t v) noexcept {
    if constexpr (detail::is_big_endian) {
        return v;
    } else {
        return __builtin_bswap16(v);
    }
}

constexpr inline std::uint32_t hton(std::uint32_t v) noexcept {
    if constexpr (detail::is_big_endian) {
        return v;
    } else {
        return __builtin_bswap32(v);
    }
}

constexpr inline std::uint64_t hton(std::uint64_t v) noexcept {
    if constexpr (detail::is_big_endian) {
        return v;
    } else {
        return __builtin_bswap64(v);
    }
}

/// Converts from network byteorder to host byteorder
constexpr inline std::uint8_t ntoh(std::uint8_t v) noexcept {
    return v;
}

constexpr inline std::uint16_t ntoh(std::uint16_t v) noexcept {
    if constexpr (detail::is_big_endian) {
        return v;
    } else {
        return __builtin_bswap16(v);
    }
}

constexpr inline std::uint32_t ntoh(std::uint32_t v) noexcept {
    if constexpr (detail::is_big_endian) {
        return v;
    } else {
        return __builtin_bswap32(v);
    }
}

constexpr inline std::uint64_t ntoh(std::uint64_t v) noexcept {
    if constexpr (detail::is_big_endian) {
        return v;
    } else {
        return __builtin_bswap64(v);
    }
}

} // namespace bipolar

#endif
