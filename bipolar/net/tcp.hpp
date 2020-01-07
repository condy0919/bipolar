//! TCP building blocks
//!
//! - `TcpStream`
//! - `TcpListener`
//!

#ifndef BIPOLAR_NET_TCP_HPP_
#define BIPOLAR_NET_TCP_HPP_

#include <netinet/tcp.h>

#include <tuple>
#include <utility>

#include "bipolar/core/movable.hpp"
#include "bipolar/core/result.hpp"
#include "bipolar/core/void.hpp"
#include "bipolar/net/socket_address.hpp"

namespace bipolar {
/// TcpStream
///
/// A TCP stream between a local and a remote socket with RAII semantics.
///
/// After creating a `TcpStream` by either `connect`ing to a remote host or
/// `accept`ing a connection on a `TcpListener`, data can be transmitted
/// by reading and writing to it.
///
/// The reading and writing portions of the connections can also be shutdown
/// individually with the `shutdown` method.
///
/// # Examples
///
/// ```
/// auto stream = TcpStream::connect(
///     SocketAddress::from_str("127.0.0.1:8080").value())
///         .expect("couldn't connect to 127.0.0.1:8080");
///
/// char buf[] = "buzz";
/// stream.write(buf, 4);
/// stream.read(buf, 4);
/// ```
class TcpStream final : public Movable {
public:
    /// Constructs a TCP stream from native handle (file descriptor).
    /// Ownership transfers.
    explicit TcpStream(int fd) noexcept : fd_(fd) {}

    /// Constructs from the given `TcpStream`, leaving it invalid
    TcpStream(TcpStream&& rhs) noexcept : fd_(rhs.fd_) {
        rhs.fd_ = -1;
    }

    TcpStream& operator=(TcpStream&& rhs) noexcept {
        TcpStream(std::move(rhs)).swap(*this);
        return *this;
    }

    /// Close if it's valid.
    ///
    /// A stream is valid only if the numeric value of fd is larger than or
    /// equal to 0.
    ///
    /// Use -1 as an invalid state internally.
    ~TcpStream() noexcept;

    /// Creates a new independently owned handle to the underlying socket.
    ///
    /// The returned `TcpStream` is a reference to the same stream that this
    /// object references. Both handles will read and write with the same
    /// stream of data, and options set on one stream will be propagated to
    /// the other stream.
    ///
    /// # Examples
    ///
    /// ```
    /// auto socket = TcpStream::connect(/* a socket address */)
    ///     .expect("couldn't connect to address");
    ///
    /// auto cloned_socket = socket.try_clone()
    ///     .expect("couldn't clone the socket");
    ///
    /// assert(socket.local_addr() == cloned_socket.local_addr());
    /// ```
    Result<TcpStream, int> try_clone() noexcept;

    /// Creates a new TCP stream and issues a **nonblocking** connect to the
    /// specified socket address.
    ///
    /// # Examples
    ///
    /// ```
    /// auto stream = TcpStream::connect(
    ///     SocketAddress::from_str("127.0.0.1:8080").value());
    /// if (stream.has_value()) {
    ///     std::puts("Connected");
    /// } else {
    ///     std::puts("Counldn't connect to server...");
    /// }
    /// ```
    static Result<TcpStream, int> connect(const SocketAddress& sa) noexcept;

    /// Sends data on the socket to the peer.
    /// On success, returns the number of bytes written.
    ///
    /// `man 2 send` for more information.
    Result<std::size_t, int> send(const void* buf, std::size_t len,
                                  int flags = 0) noexcept;

    /// An alias of `send`
    Result<std::size_t, int> write(const void* buf, std::size_t len) noexcept {
        return send(buf, len);
    }

    /// Sends data on the socket to the peer.
    /// On success, returns the number of bytes written.
    ///
    /// `man 2 writev` for more information.
    Result<std::size_t, int> writev(const struct iovec* iov,
                                    std::size_t vlen) noexcept;

    /// Sends a message to the peer.
    /// On success, returns the number of bytes written.
    ///
    /// `man 2 sendmsg` for more information.
    Result<std::size_t, int> sendmsg(const struct msghdr* msg,
                                     int flags = 0) noexcept;

    /// Receives data from the peer.
    /// On success, returns the number of bytes read.
    ///
    /// `man 2 recv` for more information.
    Result<std::size_t, int> recv(void* buf, std::size_t len,
                                  int flags = 0) noexcept;

    /// An alias of `recv`
    Result<std::size_t, int> read(void* buf, std::size_t len) noexcept {
        return recv(buf, len);
    }

    /// Receives data from the peer.
    /// On success, returns the number of bytes read.
    ///
    /// `man 2 readv` for more information.
    Result<std::size_t, int> readv(struct iovec* iov,
                                   std::size_t vlen) noexcept;

    /// Receives a message from the peer.
    /// On success, returns the number of bytes read.
    ///
    /// `man 2 recvmsg` for more information.
    Result<std::size_t, int> recvmsg(struct msghdr* msg,
                                     int flags = 0) noexcept;

    /// Returns the socket address of the local half of this TCP connection
    Result<SocketAddress, int> local_addr() noexcept;

    /// Returns the socket address of the remote peer of this TCP connection
    Result<SocketAddress, int> peer_addr() noexcept;

    /// Closes the TCP socket
    Result<Void, int> close() noexcept;

    /// Shutdowns the read, write, or both halves of ths connection.
    ///
    /// `man 2 shutdown` for more information.
    Result<Void, int> shutdown(int how) noexcept;

    /// Sets the value of the `TCP_NODELAY` option on this socket.
    ///
    /// If set, this option disables the Nagle algorithm. This means that
    /// segments are always sent as soon as possible, even if there is only a
    /// small amount of data. When not set, data is buffered until there is a
    /// sufficient amount to send out, thereby avoiding the frequent sending
    /// of small packets.
    Result<Void, int> set_nodelay(bool enable) noexcept;

    /// Gets the value of the `TCP_NODELAY` option on this socket
    Result<bool, int> nodelay() noexcept;

    /// Gets the value of the `SO_ERROR` option on this socket.
    ///
    /// This will retrive the stored error in the underlying socket, clearing
    /// the filed in the process. This can be useful for checking errors
    /// between calls.
    Result<int, int> take_error() noexcept;

    /// Gets the value of the `TCP_INFO` option on this socket.
    Result<struct tcp_info, int> get_tcp_info() noexcept;

    /// Returns the underlying file descriptor.
    ///
    /// NOTE:
    ///
    /// The returned fd may be invalidated after some methods such as
    /// `operator=`
    [[nodiscard]] int as_fd() const noexcept {
        return fd_;
    }

    /// Returns the underlying file descriptor and leaving it invalid.
    [[nodiscard]] int into_fd() noexcept {
        return std::exchange(fd_, -1);
    }

    /// Swaps
    void swap(TcpStream& rhs) noexcept {
        std::swap(fd_, rhs.fd_);
    }

private:
    int fd_;
};

/// TcpListener
///
/// A TCP listener with RAII semantics.
class TcpListener final : public Movable {
public:
    /// Constructs a TCP listener from native handle (file descriptor).
    /// Ownership transfers.
    explicit TcpListener(int fd) noexcept : fd_(fd) {}

    /// Constructs from the given `TcpListener`, leaving it invalid
    TcpListener(TcpListener&& rhs) noexcept : fd_(rhs.fd_) {
        rhs.fd_ = -1;
    }

    TcpListener& operator=(TcpListener&& rhs) noexcept {
        TcpListener(std::move(rhs)).swap(*this);
        return *this;
    }

    /// Close if it's valid.
    ///
    /// A stream is valid only if the numeric value of fd is larger than or
    /// equal to 0.
    ///
    /// Use -1 as an invalid state internally.
    ~TcpListener() noexcept;

    /// Creates a new independently owned handle to the underlying socket.
    ///
    /// The returned `TcpListener` is a reference to the same socket that
    /// this object reference. Both handle can be used to accept incoming
    /// connections and options set on one listener will affect the other.
    Result<TcpListener, int> try_clone() noexcept;

    /// Closes the TCP listener
    Result<Void, int> close() noexcept;

    /// Binds a new TCP listener which will be bound to the specified
    /// address.
    ///
    /// The returned listener is ready for accepting connections.
    ///
    /// Binding with a port number of 0 will request that the OS assigns a
    /// port to this listener. The port allocated can be queried via the
    /// `local_addr` method.
    ///
    /// `man 2 bind/listen` for more information.
    ///
    /// NOTE:
    ///
    /// 1. `SO_REUSEADDR` and `SO_REUSEPORT` are set for convenience
    /// 2. `listen` with a large backlog value which is equal to
    ///    `std::numeric_limits<int>::max()`
    static Result<TcpListener, int> bind(const SocketAddress& sa) noexcept;

    /// Accepts a new `TcpStream`.
    /// On success, returns the `TcpStream` with associated address.
    /// On failure, returns the errno. Be cautious with `EAGAIN` errno.
    ///
    /// `man 2 accept` for more information.
    Result<std::tuple<TcpStream, SocketAddress>, int> accept() noexcept;

    /// Returns the local socket address of this listener
    Result<SocketAddress, int> local_addr() noexcept;

    /// Gets the value of the `SO_ERROR` option on this socket.
    ///
    /// This will retrive the stored error in the underlying socket, clearing
    /// the filed in the process. This can be useful for checking errors
    /// between calls.
    Result<int, int> take_error() noexcept;

    /// Returns the underlying file descriptor.
    ///
    /// NOTE:
    ///
    /// The returned fd may be invalidated after some methods such as
    /// `operator=`
    [[nodiscard]] int as_fd() const noexcept {
        return fd_;
    }

    /// Returns the underlying file descriptor and leaving it invalid.
    [[nodiscard]] int into_fd() noexcept {
        return std::exchange(fd_, -1);
    }

    /// Swaps
    void swap(TcpListener& rhs) noexcept {
        std::swap(fd_, rhs.fd_);
    }

private:
    int fd_;
};
} // namespace bipolar

#endif
