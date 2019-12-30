//! IPv4Address, IPv6Address and IPAddress
//!
//! see `IPv4Address`, `IPv6Address` and `IPAddress` for details

#ifndef BIPOLAR_NET_IP_ADDRESS_HPP_
#define BIPOLAR_NET_IP_ADDRESS_HPP_

#include <netinet/in.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string_view>
#include <type_traits>
#include <variant>

#include "bipolar/core/byteorder.hpp"
#include "bipolar/core/option.hpp"
#include "bipolar/core/overload.hpp"
#include "bipolar/core/result.hpp"

namespace bipolar {
// forward
class IPAddress;
class IPv4Address;
class IPv6Address;

/// IPAddressFormatError
///
/// IP address format error
enum class IPAddressFormatError {
    INVALID_IP,
};

/// IPv6Address
///
/// # Brief
///
/// An IPv6 address
///
/// IPv6 addresses are defined as 128-bit integers in RFC4291
/// They are usually represented as eight 16-bit segments.
///
/// The size of an `IPv6Address` may vary depending on the target operating
/// system.
///
/// # Textual representation
///
/// `IPv6Address` provides a `from_str`. There are man ways to represent
/// an IPv6 address in text, but in general, each segments is written in
/// hexadecimal notation, and segments are separated by `:`. For more
/// information, see RFC5952
///
/// # Examples
///
/// ```
/// const auto localhost = IPv6Address(0, 0, 0, 0, 0, 0, 0, 1);
/// assert(localhost.is_loopback());
/// assert(localhost.str() == "::1");
/// ```
class IPv6Address {
public:
    /// Creates an unspecified IPv6 address
    ///
    /// # Examples
    ///
    /// ```
    /// IPv6Address v6;
    /// assert(v6.str() == "::");
    /// ```
    constexpr IPv6Address() noexcept : addr_(0, 0, 0, 0) {}

    /// Creates a new IPv6 address from the native type
    ///
    /// # Examples
    ///
    /// ```
    /// struct in6_addr addr = IN6ADDR_LOOPBACK_INIT;
    /// IPv6Address v6(addr);
    /// assert(v6.str() == "::1");
    /// ```
    explicit constexpr IPv6Address(struct in6_addr addr) noexcept
        : addr_(addr) {}

    /// Creates a new IPv6 address from sixteen 8-bit octets
    ///
    /// # Examples
    ///
    /// ```
    /// IPv6Address v6(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    /// assert(v6.str() == "1:203:405:607:809:a0b:c0d:e0f");
    /// ```
    template <
        typename... Ts,
        std::enable_if_t<
            sizeof...(Ts) == 16 &&
                std::conjunction_v<std::is_convertible<Ts, std::uint8_t>...>,
            int> = 0>
    constexpr IPv6Address(Ts... u8s) noexcept : addr_(std::uint8_t(u8s)...) {}

    /// Creates a new IPv6 address from eight 16-bit segments
    ///
    /// # Examples
    ///
    /// ```
    /// IPv6Address v6(hton(0x0011), hton(0x2233), hton(0x4455), hton(0x6677),
    ///                hton(0x8899), hton(0xaabb), hton(0xccdd), hton(0xeeff));
    /// assert(v6.str() == "11:2233:4455:6677:8899:aabb:ccdd:eeff");
    /// ```
    template <
        typename... Ts,
        std::enable_if_t<
            sizeof...(Ts) == 8 &&
                std::conjunction_v<std::is_convertible<Ts, std::uint16_t>...>,
            int> = 0>
    constexpr IPv6Address(Ts... u16s) noexcept
        : addr_(std::uint16_t(u16s)...) {}

    /// Creates a new IPv6 address from four 32-bit words
    template <
        typename... Ts,
        std::enable_if_t<
            sizeof...(Ts) == 4 &&
                std::conjunction_v<std::is_convertible<Ts, std::uint32_t>...>,
            int> = 0>
    constexpr IPv6Address(Ts... u32s) noexcept
        : addr_(std::uint32_t(u32s)...) {}

    /// Creates a new IPv6Address from a `string_view`
    ///
    /// # Note
    ///
    /// The string should be null-terminated due to inet_pton restriction
    ///
    /// # Examples
    ///
    /// ```
    /// const auto r1 = IPv6Address::from_str("127.0.0");
    /// assert(r1.is_err() && r1.error() == IPAddressFormatError::INVALID_IP);
    ///
    /// const auto r2 = IPv6Address::from_str("::1");
    /// assert(r2.is_ok() && r2.value().str() == "::1");
    /// ```
    static Result<IPv6Address, IPAddressFormatError>
    from_str(std::string_view sv) noexcept;

    /// Returns the eight 16-bit segments that make up this address
    ///
    /// # Examples
    ///
    /// ```
    /// IPv6Address v6;
    /// assert(v6.segments() == {});
    /// ```
    [[nodiscard]] const std::array<std::uint16_t, 8> segments() const {
        return reinterpret_cast<const std::array<std::uint16_t, 8>&>(addr_);
    }

    /// Returns the sixteen 8-bit integers the IPv6 address consists of
    ///
    /// # Examples
    ///
    /// ```
    /// IPv6Address v6;
    /// assert(v6.octets()[0] == 0);
    ///
    /// auto localhost = IPv6Address::from_str("::1").value();
    /// assert(localhost.octets()[15] == 1);
    /// ```
    [[nodiscard]] const std::array<std::uint8_t, 16> octets() const {
        return reinterpret_cast<const std::array<std::uint8_t, 16>&>(addr_);
    }

    /// Returns `true` for the special unspecified address (::)
    ///
    /// # Examples
    ///
    /// ```
    /// IPv6Address v6;
    /// assert(v6.is_unspecified());
    ///
    /// auto localhost = IPv6Address::from_str("::1").value();
    /// assert(!localhost.is_unspecified());
    /// ```
    [[nodiscard]] constexpr bool is_unspecified() const noexcept {
        return addr_.addr32[0] == 0 && addr_.addr32[1] == 0 &&
               addr_.addr32[2] == 0 && addr_.addr32[3] == 0;
    }

    /// Returns `true` if this is a loopback address (::1)
    ///
    /// # Examples
    ///
    /// ```
    /// IPv6Address v6;
    /// assert(!v6.is_loopback());
    ///
    /// auto localhost = IPv6Address::from_str("::1").value();
    /// assert(localhost.is_loopback());
    /// ```
    [[nodiscard]] constexpr bool is_loopback() const noexcept {
        return addr_.addr32[0] == 0 && addr_.addr32[1] == 0 &&
               addr_.addr32[2] == 0 &&
               addr_.addr32[3] == hton(std::uint32_t(1));
    }

    /// Converts this address to IPv4Address.
    /// Returns `None` if this address is neither IPv4-compatible or IPv4-mapped
    ///
    /// # Examples
    ///
    /// ```
    /// auto v6 = IPv6Address::from_str("::1").value();
    /// auto v4 = v6.to_ipv4().value();
    /// assert(v4.str() == "0.0.0.1");
    /// ```
    [[nodiscard]] Option<IPv4Address> to_ipv4() const;

    /// Returns the native type struct `in6_addr`
    [[nodiscard]] constexpr struct in6_addr native() const noexcept {
        return addr_.native;
    }

    /// Converts to sockaddr to communicate with system call
    [[nodiscard]] constexpr struct sockaddr_in6 to_sockaddr() const noexcept {
        struct sockaddr_in6 addr = {
            .sin6_family = AF_INET6,
            .sin6_addr = addr_.native,
        };
        return addr;
    }

    /// Stringify an IPv6 address
    ///
    /// # Examples
    ///
    /// ```
    /// IPv6Address v6;
    /// assert(v6.str() == "::");
    /// ```
    [[nodiscard]] std::string str() const;

private:
    union AddressStorage {
        static_assert(sizeof(struct in6_addr) == sizeof(std::uint32_t) * 4);

        explicit constexpr AddressStorage(struct in6_addr addr) noexcept
            : native(addr) {}

        constexpr AddressStorage(std::uint8_t a0, std::uint8_t a1,
                                 std::uint8_t a2, std::uint8_t a3,
                                 std::uint8_t a4, std::uint8_t a5,
                                 std::uint8_t a6, std::uint8_t a7,
                                 std::uint8_t a8, std::uint8_t a9,
                                 std::uint8_t a10, std::uint8_t a11,
                                 std::uint8_t a12, std::uint8_t a13,
                                 std::uint8_t a14, std::uint8_t a15) noexcept
            : addr8{a0, a1, a2,  a3,  a4,  a5,  a6,  a7,
                    a8, a9, a10, a11, a12, a13, a14, a15} {}

        constexpr AddressStorage(std::uint16_t a0, std::uint16_t a1,
                                 std::uint16_t a2, std::uint16_t a3,
                                 std::uint16_t a4, std::uint16_t a5,
                                 std::uint16_t a6, std::uint16_t a7) noexcept
            : addr16{a0, a1, a2, a3, a4, a5, a6, a7} {}

        constexpr AddressStorage(std::uint32_t a0, std::uint32_t a1,
                                 std::uint32_t a2, std::uint32_t a3) noexcept
            : addr32{a0, a1, a2, a3} {}

        struct in6_addr native;
        std::uint8_t addr8[16];
        std::uint16_t addr16[8];
        std::uint32_t addr32[4];
    } addr_;
};

/// Compares IPv6Address with other IPv6Address
inline bool operator==(const IPv6Address& lhs,
                       const IPv6Address& rhs) noexcept {
    return lhs.segments() == rhs.segments();
}

inline bool operator!=(const IPv6Address& lhs,
                       const IPv6Address& rhs) noexcept {
    return !(lhs == rhs);
}

inline bool operator<(const IPv6Address& lhs, const IPv6Address& rhs) noexcept {
    return lhs.segments() < rhs.segments();
}

inline bool operator>(const IPv6Address& lhs, const IPv6Address& rhs) noexcept {
    return rhs < lhs;
}

inline bool operator<=(const IPv6Address& lhs,
                       const IPv6Address& rhs) noexcept {
    return !(lhs > rhs);
}

inline bool operator>=(const IPv6Address& lhs,
                       const IPv6Address& rhs) noexcept {
    return !(lhs < rhs);
}

/// IPv4Address
///
/// # Brief
///
/// An IPv4 address
///
/// IPv4 addresses are defined as 32-bit integers in RFC791.
/// They are usually represented as four octets.
///
/// The size of an `IPv4Address` may vary depending on the target operating
/// system.
///
/// # Textual representation
///
/// `IPv4Address` provides a `from_str`. The four octets are in decimal
/// notation, divided by `.` (this is called "dot-decimal notation").
///
/// # Examples
///
/// ```
/// const auto localhost = IPv4Address(127, 0, 0, 1);
/// assert(localhost.is_loopback());
/// assert(localhost.str() == "127.0.0.1");
/// ```
class IPv4Address {
public:
    /// Creates an unspecified IPv4 address
    ///
    /// # Examples
    ///
    /// ```
    /// IPv4Address v4;
    /// assert(v4.str() == "0.0.0.0");
    /// ```
    constexpr IPv4Address() noexcept : IPv4Address(0) {}

    /// Creates a new IPv4Address from an `std::uint32_t` in network byteorder
    ///
    /// # Examples
    ///
    /// ```
    /// IPv4Address v4(hton(0x7f000001));
    /// assert(v4.str() == "127.0.0.1");
    /// ```
    explicit constexpr IPv4Address(std::uint32_t ip) noexcept : addr_(ip) {}

    /// Creates a new IPv4Address from the native type
    ///
    /// # Examples
    ///
    /// ```
    /// struct in_addr addr = {INADDR_BROADCAST};
    /// IPv4Address v4(addr);
    /// assert(v4.str() == "255.255.255.255");
    /// ```
    explicit constexpr IPv4Address(struct in_addr addr) noexcept
        : addr_(addr) {}

    /// Creates a new IPv4Address from four eight-bit octets
    ///
    /// The result will represent the IP address `a.b.c.d`
    ///
    /// # Examples
    ///
    /// ```
    /// IPv4Address v4(127, 0, 0, 1);
    /// assert(v4.str() == "127.0.0.1");
    /// ```
    constexpr IPv4Address(std::uint8_t a, std::uint8_t b, std::uint8_t c,
                          std::uint8_t d) noexcept
        : IPv4Address(hton((std::uint32_t(a) << 24) | (std::uint32_t(b) << 16) |
                           (std::uint32_t(c) << 8) | (std::uint32_t(d)))) {}

    /// Creates a new IPv4Address from a `string_view`
    ///
    /// # Note
    ///
    /// The string should be null-terminated due to inet_pton restriction
    ///
    /// # Examples
    ///
    /// ```
    /// const auto r1 = IPv4Address::from_str("127.0.0");
    /// assert(r1.is_err() && r1.error() == IPAddressFormatError::INVALID_IP);
    ///
    /// const auto r2 = IPv4Address::from_str("0.0.0.0");
    /// assert(r2.is_ok() && r2.value().str() == "0.0.0.0");
    /// ```
    static Result<IPv4Address, IPAddressFormatError>
    from_str(std::string_view sv) noexcept;

    /// Returns the four eight-bit integers that make up this address
    ///
    /// # Examples
    ///
    /// ```
    /// IPv4Address localhost(127, 0, 0, 1);
    /// assert(localhost.octets() == (std::array<std::uint8_t, 4>{127,0,0,1}));
    /// ```
    [[nodiscard]] const std::array<std::uint8_t, 4> octets() const {
        return reinterpret_cast<const std::array<std::uint8_t, 4>&>(addr_);
    }

    /// Returns `true` for the special unspecified address (0.0.0.0)
    ///
    /// # Examples
    ///
    /// ```
    /// IPv4Address v4(127, 0, 0, 1);
    /// assert(!v4.is_unspecified());
    ///
    /// assert(IPv4Address().is_unspecified());
    /// ```
    [[nodiscard]] constexpr bool is_unspecified() const noexcept {
        return addr_.addr32 == 0;
    }

    /// Returns `true` if this is a loopback address (127.0.0.0/8)
    ///
    /// # Examples
    ///
    /// ```
    /// IPv4Address v4(127, 0, 0, 1);
    /// assert(v4.is_loopback());
    ///
    /// assert(!IPv4Address().is_loopback());
    /// ```
    [[nodiscard]] constexpr bool is_loopback() const noexcept {
        return addr_.addr8[0] == 127;
    }

    /// Converts this address to an IPv4-compatible [IPv6 address]
    ///
    /// a.b.c.d becomes: ::a.b.c.d
    [[nodiscard]] IPv6Address to_ipv6_compatible() const noexcept {
        return IPv6Address(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, addr_.addr8[0],
                           addr_.addr8[1], addr_.addr8[2], addr_.addr8[3]);
    }

    /// Converts this address to an IPv4-mapped [IPv6 address]
    ///
    /// a.b.c.d becomes: ::ffff:a.b.c.d
    [[nodiscard]] IPv6Address to_ipv6_mapped() const noexcept {
        return IPv6Address(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff,
                           addr_.addr8[0], addr_.addr8[1], addr_.addr8[2],
                           addr_.addr8[3]);
    }

    /// Returns the native type struct `in_addr`
    [[nodiscard]] struct in_addr native() const noexcept {
        return addr_.native;
    }

    /// Returns the std::uint32_t (network byteorder) representation of
    /// the address
    ///
    /// # Examples
    ///
    /// ```
    /// IPv4Address v4(0x7f000001);
    /// assert(v4.to_long() == 0x7f000001);
    /// ```
    [[nodiscard]] constexpr std::uint32_t to_long() const noexcept {
        return addr_.addr32;
    }

    /// Converts to sockaddr to communicate with system calls
    [[nodiscard]] constexpr struct sockaddr_in to_sockaddr() const noexcept {
        struct sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_addr = addr_.native,
        };
        return addr;
    }

    /// Stringify an IPv4 address
    ///
    /// # Examples
    ///
    /// ```
    /// IPv4Address localhost(127, 0, 0, 1);
    /// assert(localhost.str() == "127.0.0.1");
    /// ```
    [[nodiscard]] std::string str() const;

private:
    union AddressStorage {
        static_assert(sizeof(struct in_addr) == sizeof(std::uint32_t));

        explicit constexpr AddressStorage(struct in_addr addr) noexcept
            : native(addr) {}

        explicit constexpr AddressStorage(std::uint32_t x) noexcept
            : addr32(x) {}

        struct in_addr native;
        std::uint8_t addr8[4];
        std::uint32_t addr32;
    } addr_;
};

/// Compares IPv4Address with other IPv4Address
inline constexpr bool operator==(const IPv4Address& lhs,
                                 const IPv4Address& rhs) noexcept {
    return lhs.to_long() == rhs.to_long();
}

inline constexpr bool operator!=(const IPv4Address& lhs,
                                 const IPv4Address& rhs) noexcept {
    return !(lhs == rhs);
}

inline constexpr bool operator<(const IPv4Address& lhs,
                                const IPv4Address& rhs) noexcept {
    return ntoh(lhs.to_long()) < ntoh(rhs.to_long());
}

inline constexpr bool operator>(const IPv4Address& lhs,
                                const IPv4Address& rhs) noexcept {
    return rhs < lhs;
}

inline constexpr bool operator<=(const IPv4Address& lhs,
                                 const IPv4Address& rhs) noexcept {
    return !(lhs > rhs);
}

inline constexpr bool operator>=(const IPv4Address& lhs,
                                 const IPv4Address& rhs) noexcept {
    return !(lhs < rhs);
}

/// IPAddress
///
/// # Brief
///
/// An IP address, either IPv4 or IPv6
///
/// This variant can contain either an `IPv4Address` or an `IPv6Address`, see
/// their respective documentation for more details.
///
/// # Examples
///
/// ```
/// auto localhost_v4 = IPAddress(IPv4Address(127, 0, 0, 1));
/// auto localhost_v6 = IPAddress(IPv6Address(0, 0, 0, 0, 0, 0, 0, 1));
///
/// assert(!localhost_v4.is_ipv6());
/// assert(localhost_v4.is_ipv4());
/// ```
class IPAddress {
public:
    /// Creates an `IPAddress` with an unspecified protocol
    constexpr IPAddress() noexcept : addr_(std::monostate{}) {}

    /// Creates an `IPAddress` from `IPv4Address`
    ///
    /// # Examples
    ///
    /// ```
    /// auto localhost = IPAddress(IPv4Address(127, 0, 0, 1));
    /// assert(localhost.is_ipv4());
    /// ```
    constexpr /*implicit*/ IPAddress(const IPv4Address& addr) noexcept
        : family_(AF_INET), addr_(addr) {}

    /// Creates an `IPAddress` from `IPv6Address`
    ///
    /// # Examples
    ///
    /// ```
    /// auto localhost = IPAddress(IPv6Address(0, 0, 0, 0, 0, 0, 0, 1));
    /// assert(localhost.is_ipv6());
    /// ```
    constexpr /*implicit*/ IPAddress(const IPv6Address& addr) noexcept
        : family_(AF_INET6), addr_(addr) {}

    /// `IPv4Address` assignment
    ///
    /// IPv{4,6}Address are nothrow copy constructible and destructible that
    /// guarantees no exceptions thrown.
    /// NOLINTNEXTLINE(bugprone-exception-escape)
    IPAddress& operator=(const IPv4Address& addr) noexcept {
        static_assert(std::is_nothrow_copy_constructible_v<IPv4Address>);
        static_assert(std::is_nothrow_destructible_v<IPv4Address>);
        static_assert(std::is_nothrow_destructible_v<IPv6Address>);

        family_ = AF_INET;
        addr_.emplace<IPv4Address>(addr);
        return *this;
    }

    /// `IPv6Address` assignment
    ///
    /// IPv{4,6}Address are nothrow copy constructible and destructible that
    /// guarantees no exceptions thrown.
    /// NOLINTNEXTLINE(bugprone-exception-escape)
    IPAddress& operator=(const IPv6Address& addr) noexcept {
        static_assert(std::is_nothrow_copy_constructible_v<IPv6Address>);
        static_assert(std::is_nothrow_destructible_v<IPv6Address>);
        static_assert(std::is_nothrow_destructible_v<IPv4Address>);

        family_ = AF_INET6;
        addr_.emplace<IPv6Address>(addr);
        return *this;
    }

    /// Resets to the unspecified protocol state
    ///
    /// IPv{4,6} are nothrown destructible that guarantees no exceptions thrown
    /// NOLINTNEXTLINE(bugprone-exception-escape)
    void reset() noexcept {
        static_assert(std::is_nothrow_destructible_v<IPv6Address>);
        static_assert(std::is_nothrow_destructible_v<IPv4Address>);

        family_ = AF_UNSPEC;
        addr_.emplace<std::monostate>();
    }

    /// Creates a new IPAddress from a `string_view`
    ///
    /// # Note
    ///
    /// The string should be null-terminated due to inet_pton restriction
    ///
    /// # Examples
    ///
    /// ```
    /// auto r1 = IPAddress::from_str("127.0.0.1").value();
    /// assert(r1.is_ipv4());
    ///
    /// auto r2 = IPAddress::from_str("::1").value();
    /// assert(r2.is_ipv6());
    /// ```
    static Result<IPAddress, IPAddressFormatError>
    from_str(std::string_view sv) noexcept;

    /// Returns the address family
    [[nodiscard]] constexpr int family() const noexcept {
        return family_;
    }

    /// Returns `true` if this address's protocol is unspecified
    [[nodiscard]] constexpr bool is_empty() const noexcept {
        return family_ == AF_UNSPEC;
    }

    /// Returns `true` if this address is an IPv4 address
    [[nodiscard]] constexpr bool is_ipv4() const noexcept {
        return family_ == AF_INET;
    }

    /// Returns `true` if this address is an IPv6 address
    [[nodiscard]] constexpr bool is_ipv6() const noexcept {
        return family_ == AF_INET6;
    }

    /// Visits address
    ///
    /// `F` must be invocable for `IPv4Address`, `IPv6Address` and
    /// `std::monostate` types.
    ///
    /// Uses the `Overload` trick to handle it.
    template <typename F>
    constexpr auto visit(F&& f) const {
        switch (family()) {
        case AF_INET:
            return std::forward<F>(f)(as_ipv4());

        case AF_INET6:
            return std::forward<F>(f)(as_ipv6());

        case AF_UNSPEC:
            return std::forward<F>(f)(std::monostate{});
        }
        __builtin_unreachable();
    }

    /// Returns `true` for the special unspecified address
    ///
    /// see `IPv4Address::is_unspecified` and `IPv6Address::is_unspecified`
    /// for details
    [[nodiscard]] constexpr bool is_unspecified() const {
        return visit(Overload{
            [](const IPv4Address& addr) { return addr.is_unspecified(); },
            [](const IPv6Address& addr) { return addr.is_unspecified(); },
            [](std::monostate) { return false; }});
    }

    /// Returns `true` for the loopback address
    ///
    /// see `IPv4Address::is_loopback` and `IPv6Address::is_loopback`
    /// for details
    [[nodiscard]] constexpr bool is_loopback() const {
        return visit(
            Overload{[](const IPv4Address& addr) { return addr.is_loopback(); },
                     [](const IPv6Address& addr) { return addr.is_loopback(); },
                     [](std::monostate) { return false; }});
    }

    /// Casts the variant to IPv4Address type
    ///
    /// Throws std::bad_variant_access when the IP address is IPv6
    [[nodiscard]] constexpr const IPv4Address& as_ipv4() const {
        return std::get<IPv4Address>(addr_);
    }

    /// Casts the variant to IPv6Address type
    ///
    /// Throws std::bad_variant_access when the IP address is IPv4
    [[nodiscard]] constexpr const IPv6Address& as_ipv6() const {
        return std::get<IPv6Address>(addr_);
    }

    /// Stringify address
    ///
    /// see `IPv4Address::str` and `IPv6Address::str` for details
    [[nodiscard]] std::string str() const {
        return visit(
            Overload{[](const IPv4Address& addr) { return addr.str(); },
                     [](const IPv6Address& addr) { return addr.str(); },
                     [](std::monostate) -> std::string { return ""; }});
    }

    /// Converts to sockaddr to communicate with system call
    [[nodiscard]] constexpr struct sockaddr_storage
    to_sockaddr(std::uint16_t port) const {
        struct sockaddr_storage addr = {
            .ss_family = static_cast<unsigned short>(family_),
        };

        if (is_ipv4()) {
            auto& sin = reinterpret_cast<struct sockaddr_in&>(addr);
            sin.sin_addr = as_ipv4().native();
            sin.sin_port = port;
        } else if (is_ipv6()) {
            auto& sin6 = reinterpret_cast<struct sockaddr_in6&>(addr);
            sin6.sin6_addr = as_ipv6().native();
            sin6.sin6_port = port;
        } else {
            // AF_UNSPEC, nothing to do
        }
        return addr;
    }

private:
    int family_ = AF_UNSPEC;
    std::variant<std::monostate, IPv4Address, IPv6Address> addr_;
};

/// Compares `IPAddress` with other `IPAddress`
inline bool operator==(const IPAddress& lhs, const IPAddress& rhs) {
    if (lhs.family() == rhs.family()) {
        if (lhs.is_ipv4()) {
            return lhs.as_ipv4() == rhs.as_ipv4();
        }
        return lhs.as_ipv6() == rhs.as_ipv6();
    }
    return false;
}

inline bool operator!=(const IPAddress& lhs, const IPAddress& rhs) {
    return !(lhs == rhs);
}

inline bool operator<(const IPAddress& lhs, const IPAddress& rhs) {
    if (lhs.family() == rhs.family()) {
        if (lhs.is_ipv4()) {
            return lhs.as_ipv4() < rhs.as_ipv4();
        }
        return lhs.as_ipv6() < rhs.as_ipv6();
    }
    return false;
}

inline bool operator>(const IPAddress& lhs, const IPAddress& rhs) {
    return rhs < lhs;
}

inline bool operator<=(const IPAddress& lhs, const IPAddress& rhs) {
    return !(lhs > rhs);
}

inline bool operator>=(const IPAddress& lhs, const IPAddress& rhs) {
    return !(lhs < rhs);
}

} // namespace bipolar

#endif
