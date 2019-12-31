//! A UDP socket
//!

#ifndef BIPOLAR_NET_UDP_HPP_
#define BIPOLAR_NET_UDP_HPP_

#include "bipolar/core/movable.hpp"
#include "bipolar/core/result.hpp"
#include "bipolar/core/void.hpp"
#include "bipolar/net/socket_address.hpp"

#include <cstdint>
#include <tuple>
#include <utility>

namespace bipolar {
/// UdpSocket
///
/// A User Datagram Protocol socket with RAII semantics.
///
/// After creating a `UdpSocket` by `bind`ing it to a socket address, data
/// can be `sendto` and `recvfrom` any other socket address.
///
/// After setting a remote address with `connect`, data can be sent to and
/// received from that address with `send`/`write` and `recv`/`read`.
///
/// # Notes about APIs
///
/// - `writev`/`readv` can be used in **connected** mode, write/read data
///   from previously bound address
/// - `sendmsg` can send message to another destination when address is
///   set in `msghdr` argument despite of the connected address.
///   `recvmsg` does the same.
/// - `sendmmsg` is a extension of `sendmsg` that it can be set to send data
///   to multiple addresses.
///   `recvmmsg` is similar.
///
/// # Examples
///
/// Leaving the port zero will let OS choose the proper port number for this
/// udp socket.
///
/// ```
/// auto socket = UdpSocket::bind(SocketAddress(IPv4Address(), 0))
///     .expect("couldn't bind to 0.0.0.0");
/// ```
///
/// Also it can be specified via the port number in big endian.
///
/// ```
/// auto socket =
///         UdpSocket::bind(
///             SocketAddress(IPv4Address(),
///                           hton(static_cast<std::uint16_t>(8080))))
///                 .expect("couldn't bind to 0.0.0.0:8080");
/// ```
class UdpSocket final : public Movable {
public:
    /// Constructs a UDP socket from native handle (file descriptor).
    /// Ownership transfers.
    explicit UdpSocket(int fd) noexcept : fd_(fd) {}

    /// Constructs from then given `UdpSocket`, leaving it invalid
    UdpSocket(UdpSocket&& rhs) noexcept : fd_(rhs.fd_) {
        rhs.fd_ = -1;
    }

    UdpSocket& operator=(UdpSocket&& rhs) noexcept {
        UdpSocket(std::move(rhs)).swap(*this);
        return *this;
    }

    /// Closes if it's valid.
    ///
    /// A socket is valid only if the numeric value of fd is larger than or
    /// equal to 0.
    ///
    /// Use -1 as an invalid state internally.
    ~UdpSocket() noexcept;

    /// Creates a new independently owned handle to the underlying fd.
    ///
    /// The returned `UdpSocket` is a reference to the same socket that this
    /// object references. Both handles will read and write the same port,
    /// and options set on one socket will be propagated to the other.
    ///
    /// # Examples
    ///
    /// ```
    /// auto socket = UdpSocket::bind(/* a socket address */)
    ///     .expect("couldn't bind to address");
    ///
    /// auto cloned_socket = socket.try_clone()
    ///     .expect("couldn't clone the socket");
    ///
    /// assert(socket.local_addr() == cloned_socket.local_addr());
    /// ```
    Result<UdpSocket, int> try_clone() noexcept;

    /// Creates a **nonblocking** UDP socket from the given address
    /// With `SO_REUSEPORT` option set.
    ///
    /// # Examples
    ///
    /// ```
    /// auto sockaddr = SocketAddress::from_str("127.0.0.1:0").value();
    /// auto socket = UdpSocket::bind(sockaddr).value();
    /// ```
    static Result<UdpSocket, int> bind(const SocketAddress& sa) noexcept;

    /// Connects this UDP socket to a remote address, allowing the `send` and
    /// `recv` syscalls to be used to send data and also applies filters to
    /// only receive data from the specified address.
    ///
    /// `man 2 connect` for more information.
    Result<Void, int> connect(const SocketAddress& sa) noexcept;

    /// Dissolves the association created by `connect`.
    ///
    /// `man 2 connect` for more information.
    Result<Void, int> dissolve() noexcept;

    /// Closes the socket
    Result<Void, int> close() noexcept;

    /// Sends data on the socket to the address previously bound via `connect`.
    /// On success, returns the number of bytes written.
    ///
    /// `man 2 send` for more information.
    Result<std::size_t, int> send(const void* buf, std::size_t len,
                                  int flags = 0) noexcept;

    /// Sends data on the socket to the given socket address.
    /// On success, returns the number of bytes written.
    ///
    /// This will return an error when the IP version of the local socket
    /// does not match the given socket address.
    ///
    /// `man 2 sendto` for more information.
    Result<std::size_t, int> sendto(const void* buf, std::size_t len,
                                    const SocketAddress& sa,
                                    int flags = 0) noexcept;

    /// An alias of `send`
    Result<std::size_t, int> write(const void* buf, std::size_t len) noexcept {
        return send(buf, len);
    }

    /// Sends data on the socket to the address previously bound via `connect`.
    /// On success, returns the number of bytes written.
    ///
    /// `man 2 writev` for more information.
    Result<std::size_t, int> writev(const struct iovec* iov,
                                    std::size_t vlen) noexcept;

    /// Sends a message to the address specified by `msghdr` if no previously
    /// bound address.
    /// If a peer address has been bound, the message shall be sent to the
    /// address in `msghdr` (overriding the bound peer address but not
    /// overwriting).
    /// On success, returns the number of bytes written.
    ///
    /// `man 2 sendmsg` for more information.
    Result<std::size_t, int> sendmsg(const struct msghdr* msg,
                                     int flags = 0) noexcept;

    /// Sends multiple messages on the socket using a single syscall.
    /// On success, the `msg_len` fields of successive elements of `msgvec`
    /// are updated to contain the number of bytes written.
    /// Returns the number of message sent from `msgvec`.
    ///
    /// `man 2 sendmmsg` for more information.
    Result<std::size_t, int> sendmmsg(struct mmsghdr* msgvec, std::size_t vlen,
                                      int flags = 0) noexcept;

    /// Receives data from the socket previously bound via `connect`.
    /// On success, returns the number of bytes read.
    ///
    /// The function must be called with valid byte array `buf` of sufficient
    /// size to hold the message bytes. If a message is too long to fit in
    /// the supplied buffer, and **MSG_PEEK** is not set in the **flags**
    /// argument, the excess bytes shall be discarded.
    ///
    /// `man 2 recv` for more information.
    Result<std::size_t, int> recv(void* buf, std::size_t len,
                                  int flags = 0) noexcept;

    /// Receives a single datagram message on the socket.
    /// On success, returns the number of bytes read and the origin.
    ///
    /// The function must be called with valid byte array `buf` of sufficient
    /// size to hold the message bytes. If a message is too long to fit in
    /// the supplied buffer, and **MSG_PEEK** is not set in the **flags**
    /// argument, the excess bytes shall be discarded.
    ///
    /// `man 2 recvfrom` for more information.
    Result<std::tuple<std::size_t, SocketAddress>, int>
    recvfrom(void* buf, std::size_t len, int flags = 0) noexcept;

    /// An alias of `recv`
    Result<std::size_t, int> read(void* buf, std::size_t len) noexcept {
        return recv(buf, len);
    }

    /// Receives data on the socket previously bound via `connect`.
    /// On success, returns the number of bytes read.
    ///
    /// `man 2 readv` for more information.
    Result<std::size_t, int> readv(struct iovec* iov,
                                   std::size_t vlen) noexcept;

    /// Receives a single datagram message on the socket.
    /// On success, returns the number of bytes read.
    ///
    /// `man 2 recvmsg` for more information.
    Result<std::size_t, int> recvmsg(struct msghdr* msg,
                                     int flags = 0) noexcept;

    /// Receives multiple messages on the socket using a single syscall.
    /// On success, the `msg_len` field of successive elements of `msgvec`
    /// are updated to contain the number of bytes read.
    ///
    /// `man 2 recvmmsg` for more information.
    Result<std::size_t, int> recvmmsg(struct mmsghdr* msgvec, std::size_t vlen,
                                      int flags = 0) noexcept;

    /// Returns the socket address that this socket was created from
    Result<SocketAddress, int> local_addr() noexcept;

    /// Returns the socket address of the remote peer that this socket
    /// was connected to
    Result<SocketAddress, int> peer_addr() noexcept;

    /// Gets the value of the `SO_ERROR` option on this socket.
    ///
    /// This will retrive the stored error in the underlying socket, clearing
    /// the field in the process. This can be useful for checking errors
    /// between calls.
    Result<int, int> take_error() noexcept;

    /// Returns the underlying file descriptor.
    ///
    /// NOTE:
    /// The returned fd may be invalidated after some methods such as
    /// `operator=`
    [[nodiscard]] int as_fd() const noexcept {
        return fd_;
    }

    /// Swaps
    void swap(UdpSocket& rhs) noexcept {
        std::swap(fd_, rhs.fd_);
    }

private:
    int fd_;
};

} // namespace bipolar

#endif
