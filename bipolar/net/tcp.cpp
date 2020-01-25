#include "bipolar/net/tcp.hpp"

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <limits>

#include "bipolar/core/assert.hpp"
#include "bipolar/net/internal/native_to_socket_address.hpp"

namespace bipolar {
TcpStream::~TcpStream() noexcept {
    const auto ret = close();
    BIPOLAR_ASSERT(!ret.is_error(), "tcp stream closed with error: {}",
                   ret.error());
}

Result<TcpStream, int> TcpStream::try_clone() noexcept {
    const int new_fd = ::dup(fd_);
    if (new_fd == -1) {
        return Err(errno);
    }
    return Ok(TcpStream(new_fd));
}

Result<TcpStream, int> TcpStream::connect(const SocketAddress& sa) noexcept {
    const int family = sa.addr().family();
    const int sock =
        ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sock == -1) {
        return Err(errno);
    }

    const auto addr = sa.to_sockaddr();
    const socklen_t len = sa.addr().is_ipv4() ? sizeof(struct sockaddr_in)
                                              : sizeof(struct sockaddr_in6);
    const int ret = ::connect(sock, (const struct sockaddr*)&addr, len);
    const int err = errno; // `close` may overwrite errno, so we save a copy
    if (ret == -1 && err != EINPROGRESS) {
        ::close(sock);
        return Err(err);
    }
    return Ok(TcpStream(sock));
}

Result<std::size_t, int> TcpStream::send(const void* buf, std::size_t len,
                                         int flags) noexcept {
    const ssize_t ret = ::send(fd_, buf, len, flags);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(static_cast<std::size_t>(ret));
}

Result<std::size_t, int> TcpStream::writev(const struct iovec* iov,
                                           std::size_t vlen) noexcept {
    const ssize_t ret = ::writev(fd_, iov, vlen);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(static_cast<std::size_t>(ret));
}

Result<std::size_t, int> TcpStream::sendmsg(const struct msghdr* msg,
                                            int flags) noexcept {
    const ssize_t ret = ::sendmsg(fd_, msg, flags);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(static_cast<std::size_t>(ret));
}

Result<std::size_t, int> TcpStream::recv(void* buf, std::size_t len,
                                         int flags) noexcept {
    const ssize_t ret = ::recv(fd_, buf, len, flags);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(static_cast<std::size_t>(ret));
}

Result<std::size_t, int> TcpStream::recvmsg(struct msghdr* msg,
                                            int flags) noexcept {
    const ssize_t ret = ::recvmsg(fd_, msg, flags);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(static_cast<std::size_t>(ret));
}

Result<SocketAddress, int> TcpStream::local_addr() noexcept {
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    const int ret = ::getsockname(fd_, (struct sockaddr*)&addr, &addr_len);
    if (ret == -1) {
        return Err(errno);
    }

    return internal::native_addr_to_socket_address(&addr, addr_len);
}

Result<SocketAddress, int> TcpStream::peer_addr() noexcept {
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    const int ret = ::getpeername(fd_, (struct sockaddr*)&addr, &addr_len);
    if (ret == -1) {
        return Err(errno);
    }

    return internal::native_addr_to_socket_address(&addr, addr_len);
}

Result<Void, int> TcpStream::close() noexcept {
    if (fd_ != -1) {
        const int copy_fd = std::exchange(fd_, -1);
        const int ret = ::close(copy_fd);
        if (ret == -1) {
            return Err(errno);
        }
    }
    return Ok(Void{});
}

Result<Void, int> TcpStream::shutdown(int how) noexcept {
    const int ret = ::shutdown(fd_, how);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(Void{});
}

Result<Void, int> TcpStream::set_nodelay(bool enable) noexcept {
    const int optval = static_cast<int>(enable);
    const int ret =
        ::setsockopt(fd_, SOL_SOCKET, TCP_NODELAY, &optval, sizeof(optval));
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(Void{});
}

Result<bool, int> TcpStream::nodelay() noexcept {
    int optval = 0;
    socklen_t len = sizeof(optval);
    const int ret = ::getsockopt(fd_, SOL_SOCKET, TCP_NODELAY, &optval, &len);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(static_cast<bool>(optval));
}

Result<Void, int>
TcpStream::set_linger(Option<std::chrono::seconds> s) noexcept {
    struct linger opt = {
        .l_onoff = s.has_value(),
        .l_linger = 0,
    };

    if (s.has_value()) {
        opt.l_linger = s.value().count();
    }

    const int ret = ::setsockopt(fd_, SOL_SOCKET, SO_LINGER, &opt, sizeof(opt));
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(Void{});
}

Result<Option<std::chrono::seconds>, int> TcpStream::linger() noexcept {
    struct linger opt;
    socklen_t len = sizeof(opt);
    const int ret = ::getsockopt(fd_, SOL_SOCKET, SO_LINGER, &opt, &len);
    if (ret == -1) {
        return Err(errno);
    }
    if (opt.l_onoff) {
        return Ok(Some(std::chrono::seconds(opt.l_linger)));
    } else {
        return Ok(None);
    }
}

Result<int, int> TcpStream::take_error() noexcept {
    int optval = 0;
    socklen_t len = sizeof(optval);
    const int ret = ::getsockopt(fd_, SOL_SOCKET, SO_ERROR, &optval, &len);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(optval);
}

Result<struct tcp_info, int> TcpStream::get_tcp_info() noexcept {
    struct tcp_info info;
    socklen_t len = sizeof(info);
    const int ret = ::getsockopt(fd_, SOL_TCP, TCP_INFO, &info, &len);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(info);
}

TcpListener::~TcpListener() noexcept {
    const auto ret = close();
    BIPOLAR_ASSERT(!ret.is_error(), "tcp listener closed with error: {}",
                   ret.error());
}

Result<TcpListener, int> TcpListener::try_clone() noexcept {
    const int new_fd = ::dup(fd_);
    if (new_fd == -1) {
        return Err(errno);
    }
    return Ok(TcpListener(new_fd));
}

Result<Void, int> TcpListener::close() noexcept {
    if (fd_ != -1) {
        const int copy_fd = std::exchange(fd_, -1);
        const int ret = ::close(copy_fd);
        if (ret == -1) {
            return Err(errno);
        }
    }
    return Ok(Void{});
}

Result<TcpListener, int> TcpListener::bind(const SocketAddress& sa) noexcept {
    const int family = sa.addr().family();
    const int sock =
        ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sock == -1) {
        return Err(errno);
    }

    const int optval = 1;

    int ret =
        ::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (ret == -1) {
        ::close(sock);
        return Err(errno);
    }

    ret = ::setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    if (ret == -1) {
        ::close(sock);
        return Err(errno);
    }

    const auto addr = sa.to_sockaddr();
    const socklen_t addr_len = sa.addr().is_ipv4()
                                   ? sizeof(struct sockaddr_in)
                                   : sizeof(struct sockaddr_in6);
    ret = ::bind(sock, (const struct sockaddr*)&addr, addr_len);
    if (ret == -1) {
        ::close(sock);
        return Err(errno);
    }

    // Sets the backlog with INT_MAX.
    // `somaxconn` is the only soft constraint now
    ret = ::listen(sock, std::numeric_limits<int>::max());
    if (ret == -1) {
        ::close(sock);
        return Err(errno);
    }
    return Ok(TcpListener(sock));
}

Result<std::tuple<TcpStream, SocketAddress>, int>
TcpListener::accept() noexcept {
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    const int conn = ::accept4(fd_, (struct sockaddr*)&addr, &addr_len,
                               SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (conn == -1) {
        return Err(errno);
    }
    return internal::native_addr_to_socket_address(&addr, addr_len)
        .map([conn](SocketAddress sa) {
            return std::make_tuple(TcpStream(conn), sa);
        });
}

Result<SocketAddress, int> TcpListener::local_addr() noexcept {
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    const int ret = ::getsockname(fd_, (struct sockaddr*)&addr, &addr_len);
    if (ret == -1) {
        return Err(errno);
    }

    return internal::native_addr_to_socket_address(&addr, addr_len);
}

Result<int, int> TcpListener::take_error() noexcept {
    int optval = 0;
    socklen_t len = sizeof(optval);
    const int ret = ::getsockopt(fd_, SOL_SOCKET, SO_ERROR, &optval, &len);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(optval);
}

} // namespace bipolar
