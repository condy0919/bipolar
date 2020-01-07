//! epoll
//!

#ifndef BIPOLAR_NET_EPOLL_HPP_
#define BIPOLAR_NET_EPOLL_HPP_

#include <sys/epoll.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "bipolar/core/movable.hpp"
#include "bipolar/core/result.hpp"
#include "bipolar/core/void.hpp"

namespace bipolar {
/// Epoll
///
/// An I/O event notification facility.
///
/// Epoll, from a user-space perspective, can be considered as a container of
/// two lists:
///
/// - the interest list monitoring the registered file descriptors
/// - the ready list containing the references of ready file descriptors
///
/// # Level-triggered vs edge-triggered
///
/// If a file descriptor registered with `EPOLLET` (edge-triggered) flag, it
/// will be delivered only when the state of file descriptor changes.
///
/// With level-triggered file descriptor (the default), it will be delivered
/// unless no events available.
///
/// `man 7 epoll` for more information.
class Epoll final : public Movable {
public:
    /// Constructs an epoll instance from native handle (file descriptor).
    /// Ownership transfers.
    explicit Epoll(int epfd) noexcept : epfd_(epfd) {}

    /// Constructs from the given `Epoll`, leaving it invalid
    Epoll(Epoll&& rhs) noexcept : epfd_(rhs.epfd_) {
        rhs.epfd_ = -1;
    }

    Epoll& operator=(Epoll&& rhs) noexcept {
        Epoll(std::move(rhs)).swap(*this);
        return *this;
    }

    /// Closes an epoll instance if it's valid.
    ///
    /// An epoll is valid only if the numeric value of epfd is larger than or
    /// equal to 0.
    ///
    /// Use -1 as an invalid state internally.
    ~Epoll() noexcept;

    /// Creates a new independently owned handle to the underlying socket.
    Result<Epoll, int> try_clone() noexcept;

    /// Creates an epoll instance
    static Result<Epoll, int> create() noexcept;

    /// Waits for events on the epoll instance.
    ///
    /// This function will block until either:
    ///
    /// - a file descriptor delivers an event
    /// - the call is interrupted by a signal handler
    /// - the timeout expires
    ///
    /// Note that the `timeout` interval will be rounded up to the system
    /// clock granularity, and kernel scheduling delays mean that the
    /// blocking interval may overrun by a small amount. Specifying a
    /// `timeout` of -1 causes `poll` to block indefinitely, while specifying
    /// a `timeout` equal to zero causes `poll` to return immediately, even
    /// if no events are available.
    ///
    /// On success, returns `Void` and resizes the `events` vector with the
    /// number of file descriptors ready for the requested I/O, or zero if
    /// no file descriptor became ready during the requested `timeout`
    /// milliseconds.
    ///
    /// `man 2 epoll_wait` for more information.
    ///
    /// # Examples
    ///
    /// ```
    /// // Creates a server socket
    /// const auto addr = SocketAddress::from_str("127.0.0.1:8081").value();
    /// auto server = TcpListener::bind(addr)
    ///     .expect("couldn't bind to 127.0.0.1:8081");
    ///
    /// // spawns a thread to accept the socket
    /// std::thread t([server = std::move(server)]() {
    ///     auto conn_result = server.accept();
    /// });
    ///
    /// auto epoll = Epoll::create().expect("epoll create failed");
    ///
    /// auto strm = TcpStream::connect(addr)
    ///     .expect("connect to 127.0.0.1:8081 failed");
    ///
    /// epoll.add(strm.as_fd(), nullptr, EPOLLIN | EPOLLOUT);
    ///
    /// // Waits for the socket to be ready.
    /// std::vector<struct epoll_event> events(128);
    /// for (;;) {
    ///     epoll.poll(events, std::chrono::milliseconds(-1))
    ///         .expect("epoll wait failed");
    ///
    ///     for (auto& event : events) {
    ///         if ((event.events & EPOLLOUT) && (event.data.ptr == nullptr)) {
    ///             // the socket is connected or spurious wakeup
    ///             break;
    ///         }
    ///     }
    /// }
    /// ```
    Result<Void, int> poll(std::vector<struct epoll_event>& events,
                           std::chrono::milliseconds timeout) noexcept;

    /// Adds `fd` to the interest list and associates the settings specified
    /// via `data` and `interests` with the internal file linked to `fd` where
    /// `data` can be one of following types:
    ///
    /// - `void*` or other pointer types can be implicitly cast to `void*`
    /// - `int`
    /// - `std::uint32_t`
    /// - `std::uint64_t`
    template <typename T,
              std::enable_if_t<std::is_convertible_v<T, void*> ||
                                   std::is_same_v<T, int> ||
                                   std::is_same_v<T, std::uint32_t> ||
                                   std::is_same_v<T, std::uint64_t>,
                               int> = 0>
    Result<Void, int> add(int fd, T data, std::uint32_t interests) noexcept {
        return epoll_control(EPOLL_CTL_ADD, fd, data, interests);
    }

    /// Changes the settings associated with `fd` in the interest list to
    /// the new settings specified via `data` and `interests` arguments where
    /// `data can be one of the following types:
    ///
    /// - `void*` or other pointer types can be implicitly cast to `void*`
    /// - `int`
    /// - `std::uint32_t`
    /// - `std::uint64_t`
    template <typename T,
              std::enable_if_t<std::is_convertible_v<T, void*> ||
                                   std::is_same_v<T, int> ||
                                   std::is_same_v<T, std::uint32_t> ||
                                   std::is_same_v<T, std::uint64_t>,
                               int> = 0>
    Result<Void, int> mod(int fd, T data, std::uint32_t interests) noexcept {
        return epoll_control(EPOLL_CTL_MOD, fd, data, interests);
    }

    /// Removes the target file descriptor `fd` from the interest list
    Result<Void, int> del(int fd) noexcept;

    /// Returns the underlying file descriptor
    [[nodiscard]] int as_fd() const noexcept {
        return epfd_;
    }

    /// Swaps
    void swap(Epoll& rhs) noexcept {
        std::swap(epfd_, rhs.epfd_);
    }

private:
    template <typename T>
    Result<Void, int> epoll_control(int op, int fd, T data,
                                    std::uint32_t interests) noexcept {
        struct epoll_event ev = {
            .events = interests,
        };

        if constexpr (std::is_convertible_v<T, void*>) {
            ev.data.ptr = data;
        } else if (std::is_same_v<T, int>) {
            ev.data.fd = data;
        } else if (std::is_same_v<T, std::uint32_t>) {
            ev.data.u32 = data;
        } else if (std::is_same_v<T, std::uint64_t>) {
            ev.data.u64 = data;
        }

        const int ret = ::epoll_ctl(epfd_, op, fd, &ev);
        if (ret == -1) {
            return Err(errno);
        }
        return Ok(Void{});
    }

private:
    int epfd_;
};

/// stringify epoll interests
///
/// # Examples
///
/// ```
/// assert(stringify_interests(EPOLLIN | EPOLLOUT) == "IN OUT ");
/// ```
std::string stringify_interests(int interests);
} // namespace bipolar

#endif
