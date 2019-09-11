/// \file socket_address.hpp

#ifndef BIPOLAR_IO_SOCKET_ADDRESS_HPP_
#define BIPOLAR_IO_SOCKET_ADDRESS_HPP_

#include <cstdint>
#include <string>
#include <string_view>

#include "bipolar/core/result.hpp"
#include "bipolar/net/ip_address.hpp"

namespace bipolar {
/// \enum SocketAddressFormatError
/// \brief Socket address format error
enum class SocketAddressFormatError {
    INVALID_ADDRESS,
    INVALID_PORT,
    INVALID_FORMAT,
};

/// \class SocketAddress
/// \brief An internet socket address, either IPv4 or IPv6.
///
/// Internet socket addresses consist of an IP address, a 16-bit port number,
/// as well as possibly some version-dependent additional information.
///
/// The size of \c SocketAddress instance may vary depending on the target
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
    /// \brief Creates a new \c SocketAddress from an \c IPAddress and a port
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
        : addr_(addr), port_(port) {}

    /// \brief Returns the \c IPAddress associated with this socket address
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

    /// \brief Changes the \c IPAddress associated with this socket address
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

    /// \brief Returns the port number associated with this socket address
    /// \note port is in network byteorder
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

    /// \brief Changes the port number associated with this socket address
    /// \note port shoule be in network byteorder
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

    /// \brief Creates a new \c SocketAddress from a \c string_view
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

    /// \brief Stringify the address and the port number
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

    /// \brief Converts to sockaddr to communicate with system calls
    constexpr struct sockaddr_storage to_sockaddr() const {
        return addr_.to_sockaddr(port_);
    }

private:
    IPAddress addr_;
    std::uint16_t port_;
};

/// @{
/// \brief Compares \c SocketAddress with other \c SocketAddress
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
/// @}

} // namespace bipolar

#endif
