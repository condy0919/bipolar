#include "bipolar/net/udp.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "bipolar/net/internal/native_to_socket_address.hpp"

namespace bipolar {
UdpSocket::~UdpSocket() noexcept {
    // FIXME diagnose required
    close();
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

    const int optval = 1;
    int ret =
        ::setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    if (ret == -1) {
        ::close(sock);
        return Err(errno);
    }

    const auto addr = sa.to_sockaddr();
    const socklen_t len = sa.addr().is_ipv4() ? sizeof(struct sockaddr_in)
                                              : sizeof(struct sockaddr_in6);
    ret = ::bind(sock, (const struct sockaddr*)&addr, len);
    if (ret == -1) {
        ::close(sock);
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

Result<Void, int> UdpSocket::close() noexcept {
    if (fd_ != -1) {
        const int copy_fd = std::exchange(fd_, -1);
        const int ret = ::close(copy_fd);
        if (ret == -1) {
            return Err(errno);
        }
    }
    return Ok(Void{});
}

Result<std::size_t, int> UdpSocket::send(const void* buf, std::size_t len,
                                         int flags) noexcept {
    const ssize_t ret = ::send(fd_, buf, len, flags);
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
    const ssize_t ret =
        ::sendto(fd_, buf, len, flags, (const struct sockaddr*)&addr, addr_len);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(static_cast<std::size_t>(ret));
}

Result<std::size_t, int> UdpSocket::writev(const struct iovec* iov,
                                           std::size_t vlen) noexcept {
    const ssize_t ret = ::writev(fd_, iov, vlen);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(static_cast<std::size_t>(ret));
}

Result<std::size_t, int> UdpSocket::sendmsg(const struct msghdr* msg,
                                            int flags) noexcept {
    const ssize_t ret = ::sendmsg(fd_, msg, flags);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(static_cast<std::size_t>(ret));
}

Result<std::size_t, int> UdpSocket::sendmmsg(struct mmsghdr* msgvec,
                                             std::size_t vlen,
                                             int flags) noexcept {
    const int ret = ::sendmmsg(fd_, msgvec, vlen, flags);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(static_cast<std::size_t>(ret));
}

Result<std::size_t, int> UdpSocket::recv(void* buf, std::size_t len,
                                         int flags) noexcept {
    const ssize_t ret = ::recv(fd_, buf, len, flags);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(static_cast<std::size_t>(ret));
}

Result<std::tuple<std::size_t, SocketAddress>, int>
UdpSocket::recvfrom(void* buf, std::size_t len, int flags) noexcept {
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    const ssize_t ret =
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

Result<std::size_t, int> UdpSocket::readv(struct iovec* iov,
                                          std::size_t vlen) noexcept {
    const ssize_t ret = ::readv(fd_, iov, vlen);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(static_cast<std::size_t>(ret));
}

Result<std::size_t, int> UdpSocket::recvmsg(struct msghdr* msg,
                                            int flags) noexcept {
    const ssize_t ret = ::recvmsg(fd_, msg, flags);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(static_cast<std::size_t>(ret));
}

Result<std::size_t, int> UdpSocket::recvmmsg(struct mmsghdr* msgvec,
                                             std::size_t vlen,
                                             int flags) noexcept {
    const int ret = ::recvmmsg(fd_, msgvec, vlen, flags, nullptr);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(static_cast<std::size_t>(ret));
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
    socklen_t optlen = sizeof(int);
    const int ret = ::getsockopt(fd_, SOL_SOCKET, SO_ERROR, &optval, &optlen);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(optval);
}
} // namespace bipolar
