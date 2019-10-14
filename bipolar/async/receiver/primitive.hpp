//! Primitives for `Receiver`
//!
//! # Brief
//!

#ifndef BIPOLAR_ASYNC_RECEIVER_PRIMITIVE_HPP_
#define BIPOLAR_ASYNC_RECEIVER_PRIMITIVE_HPP_

#include <utility>
#include <exception>
#include <type_traits>

namespace bipolar {
namespace detail {
// Supports methods on a class reference
template <typename R, typename... Vs,
          typename = decltype(std::declval<R&>().value(std::declval<Vs>()...))>
constexpr void set_value(R& recvr, Vs&&... vs) //
    noexcept(noexcept(recvr.value(std::forward<Vs>(vs)...))) {
    recvr.value(std::forward<Vs>(vs)...);
}

template <typename R, typename E,
          typename = decltype(std::declval<R&>().error(std::declval<E>()))>
constexpr void set_error(R& recvr, E e) //
    noexcept(noexcept(recvr.error(std::move(e)))) {
    recvr.error(std::move(e));
}

template <typename R, typename = decltype(std::declval<R&>().done())>
constexpr void set_done(R& recvr) noexcept(noexcept(recvr.done())) {
    recvr.done();
}

template <typename R, typename Up,
          typename = decltype(std::declval<R&>().starting(std::declval<Up>()))>
constexpr void set_starting(R& recvr, Up&& up) //
    noexcept(noexcept(recvr.starting(std::forward<Up>(up)))) {
    recvr.starting(std::forward<Up>(up));
}

// Supports methods on a class pointer (smart pointer included)
template <typename P, typename... Vs,
          typename = decltype(set_value(*std::declval<P>(),
                                        std::declval<Vs>()...))>
constexpr void set_value(P&& rp, Vs&&... vs) //
    noexcept(noexcept(set_value(*rp, std::forward<Vs>(vs)...))) {
    set_value(*rp, std::forward<Vs>(vs)...);
}

template <typename P, typename E,
          typename = decltype(set_error(*std::declval<P>(), std::declval<E>()))>
constexpr void set_error(P&& rp, E&& e) //
    noexcept(noexcept(set_error(*rp, std::forward<E>(e)))) {
    set_error(*rp, std::forward<E>(e));
}

template <typename P, typename = decltype(set_done(*std::declval<P>()))>
constexpr void set_done(P&& rp) noexcept(noexcept(set_done(*rp))) {
    set_done(*rp);
}

template <typename P, typename Up,
          typename = decltype(set_starting(*std::declval<P>(),
                                           std::declval<Up>()))>
constexpr void set_starting(P&& rp, Up&& up) //
    noexcept(noexcept(set_starting(*rp, std::forward<Up>(up)))) {
    set_starting(*rp, std::forward<Up>(up));
}
} // namespace detail

/// set_value
///
/// Let `set_value` be a functor to make template programming clear
///
/// TODO: uncomment `constexpr` when migrate to c++20
inline constexpr struct {
    template <typename R, typename... Vs,
              typename = decltype(detail::set_value(std::declval<R&>(),
                                                    std::declval<Vs>()...)),
              typename = decltype(detail::set_error(std::declval<R&>(),
                                                    std::current_exception()))>
    /*constexpr*/ void operator()(R& recvr, Vs&&... vs) const noexcept {
        try {
            detail::set_value(recvr, std::forward<Vs>(vs)...);
        } catch (...) {
            detail::set_error(recvr, std::current_exception());
        }
    }
} set_value;

/// set_error
///
/// Let `set_error` be a functor to make template programming clear
inline constexpr struct {
    template <typename R, typename E,
              typename = decltype(detail::set_error(std::declval<R&>(),
                                                    std::declval<E>()))>
    constexpr void operator()(R& recvr, E&& e) const
        noexcept(noexcept(detail::set_error(recvr, std::forward<E>(e)))) {
        detail::set_error(recvr, std::forward<E>(e));
    }

} set_error;

/// set_done
///
/// Let `set_done` be a functor to make template programming clear
///
/// TODO: uncomment `constexpr` when migrate to c++20
inline constexpr struct {
    template <typename R,
              typename = decltype(detail::set_done(std::declval<R&>())),
              typename = decltype(detail::set_error(std::declval<R&>(),
                                                    std::current_exception()))>
    /*constexpr*/ void operator()(R& recvr) noexcept {
        try {
            detail::set_done(recvr);
        } catch (...) {
            detail::set_error(recvr, std::current_exception());
        }
    }
} set_done;

/// set_starting
///
/// Let `set_starting` be a functor to make template programming clear
///
/// uncomment `constexpr` when migrate to c++20
inline constexpr struct {
    template <typename R, typename Up,
              typename = decltype(detail::set_starting(std::declval<R&>(),
                                                       std::declval<Up>())),
              typename = decltype(detail::set_error(std::declval<R&>(),
                                                    std::current_exception()))>
    /*constexpr*/ void operator()(R& recvr, Up&& up) noexcept {
        try {
            detail::set_starting(recvr, std::forward<Up>(up));
        } catch (...) {
            detail::set_error(recvr, std::current_exception());
        }
    }
} set_starting;

} // namespace bipolar

#endif
