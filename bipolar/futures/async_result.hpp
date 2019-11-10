//! AsyncResult
//!
//! `AsyncResult` is similar to `Result` except for allowing an empty state.
//!
//! See documentation of `Result` for more information.

#ifndef BIPOLAR_FUTURES_ASYNC_RESULT_HPP_
#define BIPOLAR_FUTURES_ASYNC_RESULT_HPP_

#include <utility>
#include <variant>
#include <type_traits>

#include "bipolar/core/traits.hpp"

namespace bipolar {
// forward
template <typename, typename>
class AsyncResult;

struct AsyncPending;

template <typename>
struct AsyncOk;

template <typename>
struct AsyncError;

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

template <typename T, typename E>
class AsyncResult {
public:
    enum class State {
        PENDING,
        OK,
        ERROR,
    };

    using value_type = T;
    using error_type = E;

    /// Creates a pending result
    constexpr AsyncResult() = default;
    constexpr /*implicit*/ AsyncResult(AsyncPending) {}

    /// @{
    /// Creates an ok result
    template <typename U,
              std::enable_if_t<std::is_constructible_v<T, U>, int> = 0>
    constexpr /*implicit*/ AsyncResult(AsyncOk<U>&& ok)
        : state_(std::in_place_index<1>, std::move(ok.value)) {}

    template <typename U,
              std::enable_if_t<std::is_constructible_v<T, U>, int> = 0>
    constexpr /*implicit*/ AsyncResult(const AsyncOk<U>& ok)
        : state_(std::in_place_index<1>, ok.value) {}
    /// @}

    /// @{
    /// Creates an error result
    template <typename U,
              std::enable_if_t<std::is_constructible_v<E, U>, int> = 0>
    constexpr /*implicit*/ AsyncResult(AsyncError<U>&& err)
        : state_(std::in_place_index<2>, std::move(err.value)) {}

    template <typename U,
              std::enable_if_t<std::is_constructible_v<E, U>, int> = 0>
    constexpr /*implicit*/ AsyncResult(const AsyncError<U>& err)
        : state_(std::in_place_index<2>, err.value) {}
    /// @}

    /// Moves from another result, leaving the other one in a pending state
    constexpr AsyncResult(AsyncResult&& rhs) : state_(std::move(rhs.state_)) {
        rhs.reset();
    }

    /// Copies another result (if copyable)
    constexpr AsyncResult(const AsyncResult&) = default;

    ~AsyncResult() = default;

    /// Assigns from another result (if copyable)
    constexpr AsyncResult& operator=(const AsyncResult&) = default;

    /// Moves from another result, leaving the other one in a pending state
    constexpr AsyncResult& operator=(AsyncResult&& rhs) {
        state_ = std::move(rhs.state_);
        rhs.reset();
        return *this;
    }

    /// Returns the state of the task's result: pending, ok or error
    constexpr State state() const {
        return static_cast<State>(state_.index());
    }

    /// Returns true if the result is not pending
    constexpr explicit operator bool() const {
        return !is_pending();
    }

    /// Returns true if the result is still in progress
    constexpr bool is_pending() const {
        return state() == State::PENDING;
    }

    /// Returns true if the task succeeded
    constexpr bool is_ok() const {
        return state() == State::OK;
    }

    /// Returns true if the task failed
    constexpr bool is_error() const {
        return state() == State::ERROR;
    }

    /// @{
    /// Gets the result's value
    /// Asserts that the result's state is `State::OK`
    constexpr T& value() {
        return std::get<1>(state_);
    }

    constexpr const T& value() const {
        return std::get<1>(state_);
    }
    /// @}

    /// @{
    /// Gets the result's error
    /// Asserts that the result's state is `State::ERROR`
    constexpr E& error() {
        return std::get<2>(state_);
    }

    constexpr const E& error() const {
        return std::get<2>(state_);
    }
    /// @}

    /// Takes the result's value, leaving it in a pending state
    /// Asserts that the result's state is `State::OK`
    constexpr T take_value() {
        auto ret = std::move(value());
        reset();
        return ret;
    }

    /// Takes the result's error, leaving it in a pending state
    /// Asserts that the result's state is `State::ERROR`
    constexpr E take_error() {
        auto ret = std::move(error());
        reset();
        return ret;
    }

    /// Resets `AsyncResult`
    constexpr void reset() {
        state_.template emplace<0>();
    }

    /// Swaps `AsyncResult`
    constexpr void swap(AsyncResult& rhs) {
        state_.swap(rhs);
    }

private:
    std::variant<std::monostate, T, E> state_;
};

} // namespace bipolar

#endif
