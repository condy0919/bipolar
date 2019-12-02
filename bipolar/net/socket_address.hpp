//! SocketAddress
//!
//! see `SocketAddress` for details

#ifndef BIPOLAR_IO_SOCKET_ADDRESS_HPP_
#define BIPOLAR_IO_SOCKET_ADDRESS_HPP_

#include <cstdint>
#include <string>
#include <string_view>

#include "bipolar/core/result.hpp"
#include "bipolar/net/ip_address.hpp"

namespace bipolar {
/// SocketAddressFormatError
///
/// Socket address format error
enum class SocketAddressFormatError {
    INVALID_ADDRESS,
    INVALID_PORT,
    INVALID_FORMAT,
};

/// SocketAddress
///
/// # Brief
///
/// An internet socket address, either IPv4 or IPv6.
///
/// Internet socket addresses consist of an IP address, a 16-bit port number,
/// as well as possibly some version-dependent additional information.
///
/// The size of `SocketAddress` instance may vary depending on the target
/// operating system.
///
/// # Examples
///
/// ```
/// SocketAddress addr(IPAddress(IPv4Address(127, 0, 0, 1)),
///                    hton(static_cast<std::uint16_t>(8080)));
///
/// assert(addr.port() == hton(8080));
/// assert(addr.addr().is_ipv4());
/// ```
class SocketAddress {
public:
    /// Creates a new `SocketAddress` from an `IPAddress` and a port
    /// number
    ///
    /// # Examples
    ///
    /// ```
    /// SocketAddress addr(IPAddress(IPv4Address(127, 0, 0, 1)),
    ///                    hton(static_cast<std::uint16_t>(8080)));
    ///
    /// assert(addr.port() == hton(static_cast<std::uint16_t>(8080)));
    /// assert(addr.addr() == (IPAddress(IPv4Address(127, 0, 0, 1))));
    /// ```
    constexpr SocketAddress(IPAddress addr, std::uint16_t port) noexcept
        : addr_(std::move(addr)), port_(port) {}

    /// Creates a new `SocketAddress` from the native `sockaddr_in` type
    constexpr explicit SocketAddress(const struct sockaddr_in* addr) noexcept
        : addr_(IPv4Address(addr->sin_addr)), port_(addr->sin_port) {}

    /// Creates a new `SocketAddress` from the native `sockaddr_in6` type
    constexpr explicit SocketAddress(const struct sockaddr_in6* addr) noexcept
        : addr_(IPv6Address(addr->sin6_addr)), port_(addr->sin6_port) {}

    /// Returns the `IPAddress` associated with this socket address
    ///
    /// # Examples
    ///
    /// ```
    /// SocketAddress addr(IPAddress(IPv4Address(127, 0, 0, 1)),
    ///                    hton(static_cast<std::uint16_t>(8080)));
    ///
    /// assert(addr.addr() == (IPAddress(IPv4Address(127, 0, 0, 1))));
    /// ```
    constexpr const IPAddress& addr() const noexcept {
        return addr_;
    }

    /// Changes the `IPAddress` associated with this socket address
    ///
    /// # Examples
    ///
    /// ```
    /// SocketAddress addr(IPAddress(IPv4Address()), 0);
    /// addr.addr(IPAddress(IPv4Address(127, 0, 0, 1)));
    ///
    /// assert(addr.addr() == IPAddress(IPv4Address(127, 0, 0, 1)));
    /// ```
    constexpr void addr(const IPAddress& addr) noexcept {
        addr_ = addr;
    }

    /// Returns the port number associated with this socket address
    ///
    /// # Note
    ///
    /// port is in network byteorder
    ///
    /// # Examples
    ///
    /// ```
    /// SocketAddress addr(IPAddress(IPv4Address()), 0);
    ///
    /// assert(addr.port() == 0);
    /// ```
    constexpr std::uint16_t port() const noexcept {
        return port_;
    }

    /// Changes the port number associated with this socket address
    ///
    /// # Note
    ///
    /// port shoule be in network byteorder
    ///
    /// # Examples
    ///
    /// ```
    /// SocketAddress addr(IPAddress(IPv4Address()), 0);
    ///
    /// addr.port(hton(static_cast<std::uint16_t>(8080)));
    /// assert(addr.port() == hton(static_cast<std::uint16_t>(8080)));
    /// ```
    constexpr void port(std::uint16_t port) noexcept {
        port_ = port;
    }

    /// Creates a new `SocketAddress` from a `string_view`
    /// The following two formats are valid:
    /// - `ip:port` for IPv4
    /// - `[ip]:port` for IPv6
    ///
    /// # Examples
    ///
    /// ```
    /// auto r1 = SocketAddress::from_str("127.0.0.1:8080").value();
    /// assert(r1.addr() == (IPAddress(IPv4Address(127, 0, 0, 1))));
    /// assert(r1.port() == hton(8080))
    ///
    /// auto r2 = SocketAddress::from_str("[::1]:8080").value();
    /// assert(r2.addr() ==
    ///        (IPAddress(IPv6Address(0, 0, 0, 0, 0, 0, 0,
    ///                               hton(static_cast<std::uint16_t>(1))))));
    /// assert(r2.port() == hton(static_cast<std::uint16_t>(8080)));
    ///
    /// // It's an exception but not advisable
    /// auto r3 = SocketAddress::from_str("::1:8080");
    /// assert(r3.has_value());
    ///
    /// auto r4 = SocketAddress::from_str("127.0.0:8080");
    /// assert(r4.has_error());
    /// ```
    static Result<SocketAddress, SocketAddressFormatError>
    from_str(std::string_view sv) noexcept;

    /// Stringify the address and the port number
    /// - `ip:port` for IPv4
    /// - `[ip]:port` for IPv6
    ///
    /// # Examples
    ///
    /// ```
    /// SocketAddress addr(IPAddress(IPv4Address(127, 0, 0, 1)),
    ///                    hton(static_cast<std::uint16_t>(8080)));
    /// assert(a1.str() == "127.0.0.1:8080");
    ///
    /// SocketAddress a2(IPAddress(IPv6Address()),
    ///                  hton(static_cast<std::uint16_t>(8086)));
    /// assert(a2.str() == "[::]:8086");
    /// ```
    std::string str() const;

    /// Converts to sockaddr to communicate with system calls
    constexpr struct sockaddr_storage to_sockaddr() const {
        return addr_.to_sockaddr(port_);
    }

private:
    IPAddress addr_;
    std::uint16_t port_;
};

/// Compares `SocketAddress` with other `SocketAddress`
inline bool operator==(const SocketAddress& lhs, const SocketAddress& rhs) {
    return (lhs.addr() == rhs.addr()) && (lhs.port() == rhs.port());
}

inline bool operator!=(const SocketAddress& lhs, const SocketAddress& rhs) {
    return !(lhs == rhs);
}

inline bool operator<(const SocketAddress& lhs, const SocketAddress& rhs) {
    return (lhs.addr() < rhs.addr()) && (lhs.port() < rhs.port());
}

inline bool operator>(const SocketAddress& lhs, const SocketAddress& rhs) {
    return rhs < lhs;
}

inline bool operator<=(const SocketAddress& lhs, const SocketAddress& rhs) {
    return !(lhs > rhs);
}

inline bool operator>=(const SocketAddress& lhs, const SocketAddress& rhs) {
    return !(lhs < rhs);
}

} // namespace bipolar

#endif
