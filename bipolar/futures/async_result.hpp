//! AsyncResult
//!
//! `AsyncResult` is a wrapper to `Result` except for allowing an empty state.
//!
//! See documentation of `Result` for more information.

#ifndef BIPOLAR_FUTURES_ASYNC_RESULT_HPP_
#define BIPOLAR_FUTURES_ASYNC_RESULT_HPP_

#include <type_traits>
#include <utility>

#include "bipolar/core/result.hpp"
#include "bipolar/core/traits.hpp"

namespace bipolar {
// forward
template <typename, typename>
class AsyncResult;

/// Represents the intermediate state of a `AsyncResult` that has not yet
/// completed.
struct AsyncPending {};

/// Represents the result of a successful task.
template <typename T>
struct AsyncOk {
    constexpr /*implicit*/ AsyncOk(T&& val) : value(std::move(val)) {}
    constexpr /*implicit*/ AsyncOk(const T& val) : value(val) {}

    T value;
};

/// Represents the result of a failed task.
template <typename E>
struct AsyncError {
    constexpr /*implicit*/ AsyncError(E&& err) : error(std::move(err)) {}
    constexpr /*implicit*/ AsyncError(const E& err) : error(err) {}

    E error;
};

namespace detail {
// AsyncResult helper traits
template <typename T>
using is_async_result = is_instantiation_of<T, AsyncResult>;

template <typename T>
inline constexpr bool is_async_result_v = is_async_result<T>::value;

// AsyncPending helper traits
template <typename T>
using is_async_pending = std::is_same<T, AsyncPending>;

template <typename T>
inline constexpr bool is_async_pending_v = is_async_pending<T>::value;

// AsyncOk helper traits
template <typename T>
using is_async_ok = is_instantiation_of<T, AsyncOk>;

template <typename T>
inline constexpr bool is_async_ok_v = is_async_ok<T>::value;

// AsyncError helper traits
template <typename T>
using is_async_error = is_instantiation_of<T, AsyncError>;

template <typename T>
inline constexpr bool is_async_error_v = is_async_error<T>::value;
} // namespace detail

/// AsyncResult
///
/// `AsyncResult` represents the result of a task which may have succeeded,
/// failed or still be in progress.
///
/// Use `AsyncPending{}`, `AsyncOk{}`, or `AsyncError{}` to initialize the
/// result.
///
/// # Examples
///
/// ```
/// AsyncResult<int, std::string> good = AsyncOk(13);
///
/// do_with_result(std::move(good));
/// assert(good.is_pending());
/// ```
template <typename T, typename E>
class AsyncResult final {
public:
    using value_type = T;
    using error_type = E;

    /// Creates a pending result
    constexpr AsyncResult() noexcept : result_(make_empty_result<T, E>()) {}
    constexpr /*implicit*/ AsyncResult(AsyncPending) noexcept : AsyncResult() {}

    /// Creates an ok result
    constexpr /*implicit*/ AsyncResult(AsyncOk<T>&& ok) noexcept(
        std::is_nothrow_move_constructible_v<T>)
        : result_(Ok(std::move(ok.value))) {}

    constexpr /*implicit*/ AsyncResult(const AsyncOk<T>& ok) noexcept(
        std::is_nothrow_copy_constructible_v<T>)
        : result_(Ok(ok.value)) {}

    /// Creates an error result
    constexpr /*implicit*/ AsyncResult(AsyncError<E>&& err) noexcept(
        std::is_nothrow_move_constructible_v<E>)
        : result_(Err(std::move(err.error))) {}

    constexpr /*implicit*/ AsyncResult(const AsyncError<E>& err) noexcept(
        std::is_nothrow_copy_constructible_v<E>)
        : result_(Err(err.error)) {}

    /// Moves from another result, leaving the other one in a pending state
    constexpr AsyncResult(AsyncResult&& rhs) : result_(std::move(rhs.result_)) {
        rhs.reset();
    }

    /// Copies/Assigns another result (if copyable)
    constexpr AsyncResult(const AsyncResult&) = default;
    constexpr AsyncResult& operator=(const AsyncResult&) = default;

    ~AsyncResult() = default;

    /// Moves from another result, leaving the other one in a pending state
    constexpr AsyncResult& operator=(AsyncResult&& rhs) {
        result_ = std::move(rhs.result_);
        rhs.reset();
        return *this;
    }

    /// Returns true if the result is not pending
    constexpr explicit operator bool() const noexcept {
        return !is_pending();
    }

    /// Returns true if the result is still in progress
    constexpr bool is_pending() const {
        return has_nothing(result_);
    }

    /// Returns true if the task succeeded
    constexpr bool is_ok() const {
        return result_.has_value();
    }

    /// Returns true if the task failed
    constexpr bool is_error() const {
        return result_.has_error();
    }

    /// Gets the result's value
    ///
    /// Throws `BadResultAccess` when `is_ok()` is false.
    constexpr T& value() {
        return result_.value();
    }

    constexpr const T& value() const {
        return result_.value();
    }

    /// Gets the result's error
    ///
    /// Throws `BadResultAccess` when `is_error()` is false.
    constexpr E& error() {
        return result_.error();
    }

    constexpr const E& error() const {
        return result_.error();
    }

    /// Takes the result's value, leaving it in a pending state
    ///
    /// Throws `BadResultAccess` when `is_ok()` is false.
    constexpr T take_value() {
        auto ret = std::move(value());
        reset();
        return ret;
    }

    /// Takes the result's error, leaving it in a pending state
    ///
    /// Throws `BadResultAccess` when `is_error()` is false.
    constexpr E take_error() {
        auto ret = std::move(error());
        reset();
        return ret;
    }

    /// Resets `AsyncResult`
    constexpr void reset() {
        result_.assign(Empty{});
    }

    /// Swaps `AsyncResult`
    constexpr void swap(AsyncResult& rhs) noexcept(
        detail::all_of_v<std::is_nothrow_swappable, T, E>) {
        result_.swap(rhs.result_);
    }

private:
    Result<T, E> result_;
};

/// Swaps
template <typename T, typename E>
inline constexpr void
swap(AsyncResult<T, E>& lhs, AsyncResult<T, E>& rhs) noexcept(
    detail::all_of_v<std::is_nothrow_swappable, T, E>) {
    lhs.swap(rhs);
}

} // namespace bipolar

#endif
