#include "bipolar/net/udp.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>

#include "bipolar/net/internal/native_to_socket_address.hpp"

namespace bipolar {
UdpSocket::~UdpSocket() noexcept {
    if (fd_ != -1) {
        ::close(fd_);
    }
}

Result<UdpSocket, int> UdpSocket::try_clone() noexcept {
    const int new_fd = ::dup(fd_);
    if (new_fd == -1) {
        return Err(errno);
    }
    return Ok(UdpSocket(new_fd));
}

Result<UdpSocket, int> UdpSocket::bind(const SocketAddress& sa) noexcept {
    const int family = sa.addr().family();
    const int sock =
        ::socket(family, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sock == -1) {
        return Err(errno);
    }

    const auto addr = sa.to_sockaddr();
    const socklen_t len = sa.addr().is_ipv4() ? sizeof(struct sockaddr_in)
                                              : sizeof(struct sockaddr_in6);
    const int ret = ::bind(sock, (const struct sockaddr*)&addr, len);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(UdpSocket(sock));
}

Result<Void, int> UdpSocket::connect(const SocketAddress& sa) noexcept {
    const auto addr = sa.to_sockaddr();
    const socklen_t addr_len = sa.addr().is_ipv4()
                                   ? sizeof(struct sockaddr_in)
                                   : sizeof(struct sockaddr_in6);
    const int ret = ::connect(fd_, (const struct sockaddr*)&addr, addr_len);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(Void{});
}

Result<std::size_t, int> UdpSocket::send(const void* buf, std::size_t len,
                                         int flags) noexcept {
    const int ret = ::send(fd_, buf, len, flags);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(static_cast<std::size_t>(ret));
}

Result<std::size_t, int> UdpSocket::sendto(const void* buf, std::size_t len,
                                           const SocketAddress& sa,
                                           int flags) noexcept {
    const auto addr = sa.to_sockaddr();
    const socklen_t addr_len = sa.addr().is_ipv4()
                                   ? sizeof(struct sockaddr_in)
                                   : sizeof(struct sockaddr_in6);
    const int ret =
        ::sendto(fd_, buf, len, flags, (const struct sockaddr*)&addr, addr_len);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(static_cast<std::size_t>(ret));
}

Result<std::size_t, int> UdpSocket::recv(void* buf, std::size_t len,
                                         int flags) noexcept {
    const int ret = ::recv(fd_, buf, len, flags);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(static_cast<std::size_t>(ret));
}

Result<std::tuple<std::size_t, SocketAddress>, int>
UdpSocket::recvfrom(void* buf, std::size_t len, int flags) noexcept {
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    const int ret =
        ::recvfrom(fd_, buf, len, flags, (struct sockaddr*)&addr, &addr_len);
    if (ret == -1) {
        return Err(errno);
    }

    return internal::native_addr_to_socket_address(&addr, addr_len)
        .map([ret](SocketAddress&& sa) {
            return std::make_tuple(static_cast<std::size_t>(ret),
                                   std::move(sa));
        });
}

Result<SocketAddress, int> UdpSocket::local_addr() noexcept {
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    const int ret = ::getsockname(fd_, (struct sockaddr*)&addr, &addr_len);
    if (ret == -1) {
        return Err(errno);
    }

    return internal::native_addr_to_socket_address(&addr, addr_len);
}

Result<SocketAddress, int> UdpSocket::peer_addr() noexcept {
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    const int ret = ::getpeername(fd_, (struct sockaddr*)&addr, &addr_len);
    if (ret == -1) {
        return Err(errno);
    }

    return internal::native_addr_to_socket_address(&addr, addr_len);
}

Result<int, int> UdpSocket::take_error() noexcept {
    int optval = 0;
    socklen_t optlen = 0;
    const int ret = ::getsockopt(fd_, SOL_SOCKET, SO_ERROR, &optval, &optlen);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(optval);
}
} // namespace bipolar
