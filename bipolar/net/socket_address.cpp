#include "bipolar/net/socket_address.hpp"

#include <arpa/inet.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <numeric>

#include "bipolar/core/byteorder.hpp"

namespace bipolar {
Result<SocketAddress, SocketAddressFormatError>
SocketAddress::from_str(std::string_view sv) noexcept {
    if (sv.size() <= 1) {
        return Err(SocketAddressFormatError::INVALID_FORMAT);
    }

    const std::string_view::size_type pos = sv.rfind(':');
    if ((pos == std::string_view::npos) || (pos == 0)) {
        return Err(SocketAddressFormatError::INVALID_FORMAT);
    }

    const std::string_view addr_str = sv.substr(0, pos),
                           port_str = sv.substr(pos + 1);
    if (port_str.size() > 5) {
        return Err(SocketAddressFormatError::INVALID_PORT);
    }
    if (std::any_of(port_str.begin(), port_str.end(),
                    [](char ch) { return !std::isdigit(ch); })) {
        return Err(SocketAddressFormatError::INVALID_PORT);
    }

    // the `port_str` will belong to [0, 99999] while `std::uint16_t`
    // belongs to [0, 65535]
    // range checks here
    std::uint32_t port = 0;
    std::from_chars(port_str.data(), port_str.data() + port_str.size(), port);
    if (port > std::numeric_limits<std::uint16_t>::max()) {
        return Err(SocketAddressFormatError::INVALID_PORT);
    }

    // due to the restrict of `inet_pton`, the `src` parameter needs to be
    // null-terminated while `std::string_view` violates.
    char addr[INET6_ADDRSTRLEN] = "";
    if (addr_str.front() == '[' && addr_str.back() == ']') {
        const auto substr = addr_str.substr(1, pos - 2);
        std::copy(substr.begin(), substr.end(), addr);
    } else {
        std::copy(addr_str.begin(), addr_str.end(), addr);
    }

    return IPAddress::from_str(addr)
        .map([port](const IPAddress& addr) -> SocketAddress {
            return SocketAddress(addr, hton(static_cast<std::uint16_t>(port)));
        })
        .map_err([](auto) -> SocketAddressFormatError {
            return SocketAddressFormatError::INVALID_ADDRESS;
        });
}

std::string SocketAddress::str() const {
    std::string ret;
    if (addr_.is_ipv4()) {
        ret.assign(addr_.str());
        ret.append(1, ':');
    } else if (addr_.is_ipv6()) {
        ret.append(1, '[');
        ret.append(addr_.str());
        ret.append(1, ']');
        ret.append(1, ':');
    } else {
        // empty
        return "";
    }

    char buf[6] = "";
    const auto [p, _] = std::to_chars(buf, buf + sizeof(buf), ntoh(port_));
    ret.append(buf, p);

    return ret;
}
} // namespace bipolar
