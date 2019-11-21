//!
//!
//!
//!

#ifndef BIPOLAR_FUTURES_FUTURE_INL_HPP_
#define BIPOLAR_FUTURES_FUTURE_INL_HPP_

#ifndef BIPOLAR_FUTURES_PROMISE_HPP_
#error "User should include promise.hpp only"
#endif

#include <cassert>
#include <variant>

#include "bipolar/core/void.hpp"

namespace bipolar {
// forward
template <typename Promise>
class FutureImpl;

/// FutureState
///
/// Describes the status of a future
enum class FutureState {
    /// The future neither holds a result nor a promise that could produce
    /// a result.
    EMPTY,
    /// The future holds a promise that may eventually produce a resut but
    /// it currently doesn't have a result. The future's promise must be
    /// invoked in order to make progress from this state.
    PENDING,
    /// The future holds a successful result
    OK,
    /// The future holds a failed result
    ERROR,
};

/// Future
///
/// A `Future` holds onto a `Promise` until it has completed then provides
/// access to its `Result`.
///
/// # Operations
///
/// A `Future` has a single owner who is responsible for setting its promise
/// or result and driving its execution. Unlike `Promise`, a future retains
/// the result produced by completion of its asynchronous task. Result retention
/// eases the implementation of combined tasks that need to await the results
/// of other tasks before proceeding.
///
/// A future can be in one of four states, depending on whether it holds:
/// - a successful result: `FutureState::OK`
/// - an error result: `FutureState::ERROR`
/// - a promise that may eventually produce a result: `FutureState::PENDING`
/// - neither: `FutureState::EMPTY`
///
/// On its own, a future is lazy; it only makes progress in response to actions
/// taken by its owner. The state of the future never changes spontaneously
/// or concurrently.
///
/// When the future's state is `FutureState::EMPTY`, its owner is responsible
/// for setting the future's promise or result thereby moving the future into
/// the pending or ready state.
///
/// When the future's state is `FutureState::PENDING`, its owner is responsible
/// for calling the future's `operator()` to invoke the promise. If the promise
/// completes and returns a result, the future will transition to the OK
/// or error state according to the result. The promise itself will then be
/// destroyed since it has fulfilled its purpose.
///
/// When the future's state is `FutureState::OK`, its owner is responsible for
/// consuming the stored value using `value()`, `take_value()`, `result()`,
/// `take_result()` or `take_ok_result()`.
///
/// When the future's state is `FutureState::ERROR`, its owner is responsible
/// for consuming the stored error using `error()`, `take_error()`, `result()`,
/// `take_result()` or `take_error_result()`.
///
/// See also `Promise` for more information about promises and their execution.
///
/// # Examples
///
///
template <typename T = Void, typename E = Void>
using Future = FutureImpl<Promise<T, E>>;

/// Future implementation details.
/// See `Future` documentation for more information.
template <typename Promise>
class FutureImpl final {
public:
    /// The type of promise held by the future
    using promise_type = Promise;

    /// The promise's result type.
    using result_type = typename Promise::result_type;

    /// The type of value produced when the promise completes successfully
    using value_type = typename result_type::value_type;

    /// The type of value produced when the promise completes with an error
    using error_type = typename result_type::error_type;

    /// `FutureImpl` is move-only
    FutureImpl(const FutureImpl&) = delete;
    FutureImpl& operator=(const FutureImpl&) = delete;

    /// Creates a future in the empty state
    constexpr FutureImpl() noexcept = default;
    constexpr explicit FutureImpl(std::nullptr_t) noexcept : FutureImpl() {}

    /// Creates a future and assigns a promise to compute its result.
    /// If the promise is empty, the future enters the empty state.
    /// Otherwise the future enters the pending state
    constexpr explicit FutureImpl(promise_type p) {
        if (p) {
            state_.template emplace<1>(std::move(p));
        }
    }

    /// Creates a future and assigns its result.
    /// If the result is pending, the future enters the empty state.
    /// Otherwise the future enters the ok or error state.
    constexpr explicit FutureImpl(result_type result) {
        if (result) {
            state_.template emplace<2>(std::move(result));
        }
    }

    /// Moves from another future, leaving the other one in an empty state.
    constexpr FutureImpl(FutureImpl&& rhs) : state_(std::move(rhs.state_)) {
        rhs.state_.template emplace<0>();
    }

    constexpr FutureImpl& operator=(FutureImpl&& rhs) {
        state_ = std::move(rhs.state_);
        rhs.state_.template emplace<0>();
        return *this;
    }

    /// Assigns the future's result.
    /// If the result is pending, the future enters the empty state.
    /// Otherwise the future enters the ok/error state.
    constexpr FutureImpl& operator=(result_type result) {
        if (!result.is_pending()) {
            state_.template emplace<2>(std::move(result));
        } else {
            state_.template emplace<0>();
        }
        return *this;
    }

    /// Assigns a promise to compute the future's result.
    /// If the promise is empty, the future enters the empty state.
    /// Otherwise the future enters the pending state.
    constexpr FutureImpl& operator=(promise_type p) {
        if (p) {
            state_.template emplace<1>(std::move(p));
        } else {
            state_.template emplace<0>();
        }
        return *this;
    }

    /// Discards the future's promise and result, leaving it empty.
    constexpr FutureImpl& operator=(std::nullptr_t) {
        state_.template emplace<0>();
        return *this;
    }

    /// Destroys the promise, releasing its promise and result (if any)
    ~FutureImpl() = default;

    /// Returns the state of the future:
    /// - EMPTY
    /// - PENDING
    /// - OK
    /// - ERROR
    constexpr FutureState state() const noexcept {
        switch (state_.index()) {
        case 0:
            return FutureState::EMPTY;

        case 1:
            return FutureState::PENDING;

        case 2:
            return std::get<2>(state_).is_ok() ? FutureState::OK
                                               : FutureState::ERROR;
        }

        __builtin_unreachable();
    }

    /// Returns true if the future's state is not `EMPTY`.
    /// It either holds a result or a promise that can be invoked to make
    /// progress towards obtaining a result.
    constexpr explicit operator bool() const noexcept {
        return !is_empty();
    }

    /// Returns true if the future's state is `EMPTY`.
    /// It does not hold a result or a promise so it cannot make progress.
    constexpr bool is_empty() const noexcept {
        return state() == FutureState::EMPTY;
    }

    /// Returns true if the future's state is `PENDING`.
    /// It does not hold a result yet but it does hold a promise
    /// that can be invoked to make progress towards obtaining a result.
    constexpr bool is_pending() const noexcept {
        return state() == FutureState::PENDING;
    }

    /// Returns true if the future's state is `OK`.
    /// It holds a value that can be retrieved using `value()`, `take_value()`,
    /// `result()`, `take_result()`, or `take_ok_result()`.
    constexpr bool is_ok() const noexcept {
        return state() == FutureState::OK;
    }

    /// Returns true if the future's state is `ERROR`.
    /// It holds an error that can be retrieved using `error()`, `take_error()`,
    /// `result()`, `take_result()`, or `take_error_result()`.
    constexpr bool is_error() const noexcept {
        return state() == FutureState::ERROR;
    }

    /// Returns true if the future's state is either `OK` or `ERROR`.
    constexpr bool is_ready() const noexcept {
        return state_.index() == 2;
    }

    /// Evaluates the future and returns true if its result is ready.
    ///
    /// If the promise completes and returns a result, the future will
    /// transition to the ok or error state according to the result. The promise
    /// itself will then be destroyed since it has fulfilled its purpose.
    constexpr bool operator()(Context& ctx) {
        switch (state_.index()) {
        case 0:
            return false;

        case 1:
            if (result_type result = std::get<1>(state_)(ctx)) {
                state_.template emplace<2>(std::move(result));
                return true;
            }
            return false;

        case 2:
            return true;
        }

        __builtin_unreachable();
    }

    /// Gets a reference to the future's promise.
    /// Asserts that the future's state is `PENDING`.
    constexpr const promise_type& promise() const {
        assert(is_pending());
        return std::get<1>(state_);
    }

    /// Takes the future's promise, leaving it in an empty state.
    /// Asserts that the future's state is `PENDING`.
    constexpr promise_type take_promise() {
        assert(is_pending());
        auto promise = std::move(std::get<1>(state_));
        state_.template emplace<0>();
        return promise;
    }

    /// Get a reference to the future's result.
    /// Asserts that the future's state is `OK` or `ERROR`.
    constexpr result_type& result() {
        assert(is_ready());
        return std::get<2>(state_);
    }

    constexpr const result_type& result() const {
        assert(is_ready());
        return std::get<2>(state_);
    }

    /// Takes the future's result, leaving it in an empty state.
    /// Asserts that the future's state is `OK` or `ERROR`.
    constexpr result_type take_result() {
        assert(is_ready());
        auto result = std::move(std::get<2>(state_));
        state_.template emplace<0>();
        return result;
    }

    /// Gets a reference to the future's value.
    /// Asserts that the future's state is `OK`.
    constexpr value_type& value() {
        assert(is_ok());
        return std::get<2>(state_).value();
    }

    constexpr const value_type& value() const {
        assert(is_ok());
        return std::get<2>(state_).value();
    }

    /// Takes the future's value, leaving it in an empty state.
    /// Asserts that the future's state is `OK`.
    constexpr value_type take_value() {
        assert(is_ok());
        auto value = std::get<2>(state_).take_value();
        state_.template emplace<0>();
        return value;
    }

    /// Gets a reference to the future's error.
    /// Asserts that the future's state is `ERROR`.
    constexpr error_type& error() {
        assert(is_error());
        return std::get<2>(state_).error();
    }

    constexpr const error_type& error() const {
        assert(is_error());
        return std::get<2>(state_).error();
    }

    /// Takes the future's error, leaving it in an empty state.
    /// Asserts that the future's state is `ERROR`.
    constexpr error_type take_error() {
        assert(is_error());
        auto error = std::get<2>(state_).take_error();
        state_.template emplace<0>();
        return error;
    }

    /// Swaps the future's contents
    constexpr void swap(FutureImpl& rhs) noexcept {
        using std::swap;
        swap(state_, rhs.state_);
    }

private:
    std::variant<std::monostate, promise_type, result_type> state_;
};

/// Swaps
template <typename T>
inline constexpr void swap(FutureImpl<T>& lhs, FutureImpl<T>& rhs) {
    lhs.swap(rhs);
}

/// Compares with `nullptr`
template <typename Promise>
inline constexpr bool operator==(const FutureImpl<Promise>& f,
                                 std::nullptr_t) noexcept {
    return !f;
}

template <typename Promise>
inline constexpr bool operator!=(const FutureImpl<Promise>& f,
                                 std::nullptr_t) noexcept {
    return static_cast<bool>(f);
}

template <typename Promise>
inline constexpr bool operator==(std::nullptr_t,
                                 const FutureImpl<Promise>& f) noexcept {
    return !f;
}

template <typename Promise>
inline constexpr bool operator!=(std::nullptr_t,
                                 const FutureImpl<Promise>& f) noexcept {
    return static_cast<bool>(f);
}

/// Makes a future containing the specified promise
template <typename Promise>
inline constexpr auto make_future(Promise p) {
    return FutureImpl<Promise>(std::move(p));
}

} // namespace bipolar

#endif
