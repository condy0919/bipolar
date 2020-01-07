#include "bipolar/net/ip_address.hpp"

#include <arpa/inet.h>

#include <cstdio>

namespace bipolar {
Result<IPv6Address, IPAddressFormatError>
IPv6Address::from_str(std::string_view sv) noexcept {
    struct in6_addr addr;
    if (::inet_pton(AF_INET6, sv.data(), &addr) != 1) {
        return Err(IPAddressFormatError::INVALID_IP);
    }
    return Ok(IPv6Address(addr));
}

Option<IPv4Address> IPv6Address::to_ipv4() const {
    if (addr_.addr16[0] || addr_.addr16[1] || addr_.addr16[2] ||
        addr_.addr16[3] || addr_.addr16[4]) {
        return None;
    }

    if ((addr_.addr16[5] != 0) && (addr_.addr16[5] != 0xffff)) {
        return None;
    }

    return Some(IPv4Address(addr_.addr8[12], addr_.addr8[13], addr_.addr8[14],
                            addr_.addr8[15]));
}

std::string IPv6Address::str() const {
    char buf[INET6_ADDRSTRLEN] = "";
    ::inet_ntop(AF_INET6, (const void*)&addr_.native, buf, INET6_ADDRSTRLEN);
    return std::string(buf);
}

Result<IPv4Address, IPAddressFormatError>
IPv4Address::from_str(std::string_view sv) noexcept {
    struct in_addr addr;
    if (::inet_pton(AF_INET, sv.data(), &addr) != 1) {
        return Err(IPAddressFormatError::INVALID_IP);
    }
    return Ok(IPv4Address(addr));
}

std::string IPv4Address::str() const {
    char buf[INET_ADDRSTRLEN] = "";
    ::inet_ntop(AF_INET, (const void*)&addr_.native, buf, INET_ADDRSTRLEN);
    return std::string(buf);
}

Result<IPAddress, IPAddressFormatError>
// `map` and `or_else` are not specified as noexcept.
// Perhaps `noexcept(auto)` is the hope.
// See https://www.reddit.com/r/cpp/comments/9ygb73/noexceptauto/ for more
// information
// NOLINTNEXTLINE(bugprone-exception-escape)
IPAddress::from_str(std::string_view sv) noexcept {
    return IPv4Address::from_str(sv)
        .map([](const IPv4Address& addr) -> IPAddress {
            return IPAddress(addr);
        })
        .or_else([sv](auto) -> Result<IPAddress, IPAddressFormatError> {
            return IPv6Address::from_str(sv).map(
                [](const IPv6Address& addr) -> IPAddress {
                    return IPAddress(addr);
                });
        });
}

} // namespace bipolar
