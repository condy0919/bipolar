#ifndef BIPOLAR_NET_INTERNAL_NATIVE_TO_SOCKET_ADDRESS_HPP_
#define BIPOLAR_NET_INTERNAL_NATIVE_TO_SOCKET_ADDRESS_HPP_

#include <netinet/in.h>

#include "bipolar/core/result.hpp"
#include "bipolar/net/socket_address.hpp"

namespace bipolar {
namespace internal {
inline static Result<SocketAddress, int>
native_addr_to_socket_address(const struct sockaddr_storage* addr,
                              socklen_t len) {
    switch (addr->ss_family) {
    case AF_INET:
        assert(len >= sizeof(struct sockaddr_in));
        return Ok(SocketAddress((const struct sockaddr_in*)addr));

    case AF_INET6:
        assert(len >= sizeof(struct sockaddr_in6));
        return Ok(SocketAddress((const struct sockaddr_in6*)addr));

    default:
        return Err(EINVAL);
    }
}

} // namespace internal
} // namespace bipolar

#endif
