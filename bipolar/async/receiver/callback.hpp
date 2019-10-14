//! Receiver related Callbacks
//!
//! # Brief

#ifndef BIPOLAR_ASYNC_RECEIVER_CALLBACK_HPP_
#define BIPOLAR_ASYNC_RECEIVER_CALLBACK_HPP_

#include <exception>
#include <utility>

#include "bipolar/core/overload.hpp"
#include "bipolar/async/receiver/primitive.hpp"

namespace bipolar {
/// Ignore
///
/// # Brief
///
struct Ignore {
    template <typename... Args>
    constexpr void operator()(Args&&...) noexcept {}
};

/// Abort
///
/// # Brief
struct Abort {
    template <typename... Args>
    [[noreturn]] void operator()(Args&&...) noexcept {
        std::terminate();
    }
};

/// SetValue
///
/// # Brief
struct SetValue {
    template <typename R, typename... Args,
              typename = decltype(set_value(std::declval<R&>(),
                                            std::declval<Args>()...))>
    constexpr void operator()(R& recvr, Args&&... args) const noexcept {
        set_value(recvr, std::forward<Args>(args)...);
    }
};

/// SetError
///
/// # Brief
struct SetError {
    template <typename R, typename E,
              typename = decltype(set_error(std::declval<R&>(),
                                            std::declval<E>()))>
    constexpr void operator()(R& recvr, E&& e) const noexcept {
        set_error(recvr, std::forward<E>(e));
    }
};

/// SetDone
///
/// # Brief
struct SetDone {
    template <typename R, typename = decltype(set_done(std::declval<R&>()))>
    constexpr void operator()(R& recvr) const noexcept {
        set_done(recvr);
    }
};

/// SetStarting
///
/// # Brief
struct SetStarting {
    template <typename R, typename Up,
              typename = decltype(set_starting(std::declval<R&>(),
                                               std::declval<Up>()))>
    constexpr void operator()(R& recvr, Up&& up) const noexcept {
        set_starting(recvr, std::forward<Up>(up));
    }
};

/// @{
/// OnValue
///
/// # Brief
template <typename... Fns>
struct OnValue : Overload<Fns...> {};

template <typename... Fns>
OnValue(Fns...) -> OnValue<Fns...>;
/// @}

/// @{
/// OnError
///
/// # Brief
template <typename... Fns>
struct OnError : Overload<Fns...> {};

template <typename... Fns>
OnError(Fns...) -> OnError<Fns...>;
/// @}

/// @{
/// OnDone
///
/// # Brief
///
template <typename Fn>
struct OnDone : Overload<Fn> {};

template <typename Fn>
OnDone(Fn) -> OnDone<Fn>;
/// @}

/// @{
/// OnStarting
///
/// # Brief
template <typename... Fns>
struct OnStarting : Overload<Fns...> {};

template <typename... Fns>
OnStarting(Fns...) -> OnStarting<Fns...>;
/// @}

} // namespace bipolar

#endif
