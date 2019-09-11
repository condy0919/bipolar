#include "bipolar/net/socket_address.hpp"
#include "bipolar/core/byteorder.hpp"

#include <arpa/inet.h>

#include <cctype>
#include <numeric>
#include <algorithm>

#include <boost/lexical_cast.hpp>

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
    const std::uint32_t port = boost::lexical_cast<std::uint32_t>(port_str);
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
        ret.append(boost::lexical_cast<std::string>(ntoh(port_)));
    } else {
        ret.append(1, '[');
        ret.append(addr_.str());
        ret.append(1, ']');
        ret.append(1, ':');
        ret.append(boost::lexical_cast<std::string>(ntoh(port_)));
    }
    return ret;
}
} // namespace bipolar
