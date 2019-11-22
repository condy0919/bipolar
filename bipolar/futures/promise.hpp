//!
//!
//!

#ifndef BIPOLAR_FUTURES_PROMISE_HPP_
#define BIPOLAR_FUTURES_PROMISE_HPP_

#include <cassert>
#include <type_traits>
#include <vector>

#include "bipolar/core/function.hpp"
#include "bipolar/core/option.hpp"
#include "bipolar/core/traits.hpp"
#include "bipolar/core/void.hpp"
#include "bipolar/futures/async_result.hpp"
#include "bipolar/futures/context.hpp"
#include "bipolar/futures/internal/adaptor.hpp"
#include "bipolar/futures/traits.hpp"

#include <boost/callable_traits.hpp>

namespace bipolar {
// forward
template <typename>
class PromiseImpl;

template <typename Continuation>
constexpr PromiseImpl<Continuation> with_continuation(Continuation);

/// Promise
///
/// # Brief
///
/// A `Promise` is a building block for asynchronous control flow that wraps
/// an asynchronous task in the form of a continuation that is repeatedly
/// invoked by an executor until it produces a result.
///
/// Additional asynchronous tasks can be chained onto the promise using
/// a variety of combinators such as `then()`.
///
/// And some helpful functions/classes:
/// - `make_promise()` creates a promise with a continuation
/// - `make_ok_promise()` creates a promise that immediately returns a value
/// - `make_error_promise()` creates a promise that immediately returns an error
/// - `make_result_promise()` creates a promise that immediately returns
///   a result
/// - `Future` more conveniently holds a promise or its result
/// - `PendingTask` wraps a promise as a pending task for execution
/// - `Executor` executes a pending task
///
/// Always look to the future; never look back.
///
/// # Chaining promises using combinators
///
/// `Promise`s can be chained together using combinators such as `then()` which
/// consume the original promise(s) and return a new combined promise.
///
/// For example, the `then()` combinator returns a promise that has the effect
/// of asynchronously awaiting completion of the prior promise (the instance
/// upon which `then()` was called) then delivering its result to a handler
/// function.
///
/// Available combinators:
/// - `then()`: run a handler when prior promise completes
/// - `and_then()`: run a handler when prior promise completes successfully
/// - `or_else()`: run a handler when prior promise completes with an error
/// - `inspect()`: examine result of prior promise
/// - `discard_result()`: discard result and unconditionally return `Result`
///                       when prior promise completes
/// - `wrap_with()`: applies a wrapper to the promise
/// - `box()`: wraps the promise's continuation into a `Function`
/// - `join_promises()`: await multiple promises in an argument list once they
///                      all complete return a tuple of their results
/// - `join_promise_vector()`: await multiple promises in a vector once they
///                            all complete return a vector of their results
///
/// # Continuations and handlers
///
/// Internally, `Promise` wraps a continuation (a kind of callable object) that
/// holds the state of the asynchronous task and provides a means for making
/// progress through repeated invocation.
///
/// A promise's continuation is generated through the use of factories such as
/// `make_promise()` and combinators such as `then()`. Most of these functions
/// accept a client-supplied "handler" (another kind of callable object, often
/// a lambda expression) which performs the actual computations.
///
/// Continuations have a very regular interface: they always accept a `Context&`
/// argument and return a `Result`. Conversely, handlers have a very flexible
/// interface: clients can provide them in many forms all of which are
/// documented by the individual functions which consume them. It's pretty easy
/// to use: the library takes care of wrapping client-supplied handlers of all
/// supported forms into the continuations it uses internally.
///
/// # Theory of operation
///
/// On its own, a promise is lazy; it only makes progress in response to actions
/// taken by its owner. The state of the promise never changes spontaneously or
/// concurrently.
///
/// Typically, a promise is executed by wrapping it into a `PendingTask` and
/// scheduling it for execution using `Executor::schedule_task()`.
/// A promise's `operator()(Context&)` can also be invoked directly by its owner
/// from within the scope of another task (this is used to implement combinators
/// and futures) though the principle is the same.
///
/// `Executor` is an abstract class that encapsulates a strategy for executing
/// tasks. The executor is responsible for invoking each tasks' continuation
/// until the task returns a non-pending result, indicating that the task has
/// been completed.
///
/// The method of execution and scheduling of each continuation call is left to
/// the discretion of each executor implementation. Typical executor
/// implementations may dispath tasks on an event-driven message loop or on a
/// thread pool. Developers are responsible for selecting appropriate executor
/// implementations for their programs.
///
/// During each invocation, the executor passes the continuation an execution
/// context object represented by a subclass of `Context`. The continuation
/// attempts to make progress then returns a value of type `AsyncResult` to
/// indicate whether it completed successfully (signaled by `AsyncOk`), failed
/// with an error (signaled by `AsyncError`), or was unable to complete the task
/// during that invocation (signaled by `AsyncPending`).
/// For example, a continuation may be unable to complete the task if it must
/// asynchronously await completion of an IO or IPC operation before it can
/// proceed any futher.
///
/// If the continuation was unable to complete the task during its invocation,
/// it may call `Context::suspend_task()` to acquire a `SuspendedTask` object.
/// The continuation then arranges for the task to be resumed asynchronously
/// (with `SuspendedTask::resume_task()`) once it becomes possible for the
/// promise to make forward progress again. Finally, the continuation returns
/// `AsyncPending` to indicate to the executor that it was unable to complete
/// the task during that invocation.
///
/// When the executor receives a pending result from a task's continuation, it
/// moves the task into a table of suspended tasks. A suspended task is
/// considered abandoned if it has not been resumed and all remaining
/// `SuspendedTask` handles representing it have been dropped. When a task is
/// abandoned, the executor removes it from its table of suspended tasks and
/// destroys the task because it is not possible for the task to be resumed
/// or to make progress from that state.
///
/// See `SingleThreadedExecutor` for a simple executor implementation.
///
/// # Boxed and unboxed promises
///
/// To make combination and execution as efficient as possible, the promises
/// returned by `make_promise()` and by combinators are parameterized by
/// complicated continuation types that are hard to describe, often consisting
/// of nested templates and lambdas. These are referred to as "unboxed"
/// promises. In contrast, "boxed" promises are parameterized by a `Function`
/// that hides (or "erases") the type of the continuation thereby yielding type
/// that is easier to describe.
///
/// You can recognize boxed and unboxed promise by their types.
/// Here are two examples:
/// - A boxed promise type: `Promise<Void, Void>` which is an alias for
///  `PromiseImpl<Function<AsyncResult<Void, Void>(Context&)>>`
/// - A unboxed promise type: `PromiseImpl<internal::ThenContinuation<...>>`
///
/// Although boxed promises are easier to manipulate, they may cause the
/// continuation to be allocated on the heap. Chaining boxed promises can result
/// in multiple allocations being produced.
///
/// Conversely, unboxed promises have full type information. Not only does this
/// defer heap allocation but is also makes it easier for the c++ compiler to
/// fuse a chains of unboxed promises together into a single object that is
/// easier to optimize.
///
/// Unboxed promises can be boxed by assigning them to a boxed promise type
/// (such as `Promise<Void, Void>`) or using the `box()` combinator.
///
/// As a rule of thumb, always defer boxing of promises until it is necessary
/// to transport them using a simpler type.
///
/// Do this: (Chaining as a single expression performs at most one heap
/// allocation)
///
/// ```
/// Promise<Void, Void> = make_promise([]() -> AsyncResult<Void, Void> { ... })
///     .then([](AsyncResult<Void, Void>& result) { ... })
///     .and_then([]() { ... });
/// ```
///
/// Or this: (still only performs at most one heap allocation)
///
/// ```
/// auto f = make_promise([]() -> AsyncResult<Void, Void> { ... });
/// auto g = f.then([](AsyncResult<Void, Void>& result) { ... });
/// auto h = g.and_then([]() { ... });
/// auto boxed_h = h.box();
/// ```
///
/// But don't do this: (incurs up to three heap allocations due to eager boxing)
///
/// ```
/// Promise<Void, Void> f = make_promise([]() { ... });
/// Promise<Void, Void> g = f.then([]() { ... });
/// Promise<Void, Void> h = g.and_then([]() { ... });
/// ```
///
/// # Single ownership model
///
/// Promises have single-ownership semantics. This means that there can only be
/// at most one reference to the task represented by its continuation along with
/// any state held by that continuation.
///
/// When a combinator is applied to a promise, ownership of its continuation
/// is transferred to the combined promise, leaving the original promise in an
/// "empty" state without a continuation. Note that it is an error to attempt
/// to invoke an empty promise (will assert at runtime).
///
/// This model greatly simplifies reasoning about object lifetime. If a promise
/// goes out of scope without completing its task, the task is considered
/// "abandoned", causing all associated state ot be destroyed.
///
/// Note that a promise may capture reference to other objects whose lifetime
/// differs from that of the promise. It is the responsibility of the promise
/// to ensure reachability of the object whose reference it capture such as by
/// using reference counted pointers, weak ppointers, or other appropriate
/// mechanisms to ensure memory safety.
///
/// # Threading model
///
/// Promise objects are not thread-safe themselves. You cannot call their
/// methods concurrently (or re-entrantly). However, promises can safely be
/// moved to other threads and executed there (unless their continuation
/// required thread affinity for some reason but that's beyond the scope
/// of this document).
///
/// This property of being thread-independent, combined with the single
/// ownership model, greatly simplifies the implementation of thread pool
/// based executors.
///
/// # Result retention and AsyncResult
///
/// A promise's continuation can only be executed to completion once.
/// After it completes, it cannot be run again.
///
/// This method of execution is very efficient; the promise's result is returned
/// directly to its invoker; it is not copied or retained within the promise
/// object itself. It is entirely the caller's responsibility to decide how to
/// consume or retain the result if needed.
///
/// For example, the caller can move the promise into a `Future` to
/// more conveniently hold either the promise or its result upon completion.
///
/// # Clarification of nomenclature
///
/// In this library, the words "promise" and "future" have the following
/// definitions:
/// - A *promise* holds the function that performs an asynchronous task.
///   It is the means to produce a value.
/// - A *future* holds the value produced by an asynchronous task or a promise
///   to produce that value if the task has not yet completed.
///   It is a proxy for a value that is to be computed.
///
/// Be aware that other libraries may use theses terms slightly differently.
///
/// For more information about the theory of futures and promises, see
/// https://en.wikipedia.org/wiki/Futures_and_promises
///
template <typename T = Void, typename E = Void>
using Promise = PromiseImpl<Function<AsyncResult<T, E>(Context&)>>;

/// See documentation of `Promise` for more information
template <typename Continuation>
class PromiseImpl final {
    // A continuation is a callable object with this signature:
    // AsyncResult<T, E>(Context&)
    static_assert(is_continuation_v<Continuation>,
                  "Continuation type in invalid");

    template <typename>
    friend class PromiseImpl;

public:
    /// The promise's result type.
    /// Equivalent to `AsyncResult<T, E>`
    using result_type = typename continuation_traits<Continuation>::result_type;

    /// The type of value produced when the promise completes successfully
    using value_type = typename result_type::value_type;

    /// The type of value produced when the promise completes with an error.
    using error_type = typename result_type::error_type;

    /// Creates an empty promise without a continuation.
    /// A continuation must be assigned before the promise can be used.
    constexpr PromiseImpl() noexcept = default;
    constexpr explicit PromiseImpl(std::nullptr_t) noexcept {}

    /// `Promise` is move-only
    PromiseImpl(const PromiseImpl&) = delete;
    PromiseImpl& operator=(const PromiseImpl&) = delete;

    /// Constructs/Assigns the promise by taking the continuation from another
    /// promise, leaving the other promise empty.
    constexpr PromiseImpl(PromiseImpl&& rhs) = default;
    constexpr PromiseImpl& operator=(PromiseImpl&& rhs) = default;

    /// Creates a promise with a continuation.
    constexpr explicit PromiseImpl(Continuation continuation) noexcept(
        std::is_nothrow_move_constructible_v<Continuation>)
        : cont_(std::move(continuation)) {}

    /// Converts from a promise holding a continuation that is assignable to
    /// this promise's continuation type
    ///
    /// This is typically used to create a promise with a boxed continuation
    /// type (such as `Function`) from an unboxed promise produced by
    /// `Promise` constructor or by combinators.
    ///
    /// # Examples
    ///
    /// ```
    /// // f is a `Promise` with a complicated unboxed type
    /// auto f = Promise([](Context&) -> Result<Void, Void> { ... });
    ///
    /// // g wraps f's continuation
    /// BoxedPromise<Void, Void> g = std::move(f);
    /// ```
    template <
        typename OtherContinuation,
        std::enable_if_t<
            !std::is_same_v<Continuation, OtherContinuation> &&
                std::is_constructible_v<Continuation, OtherContinuation&&>,
            int> = 0>
    constexpr PromiseImpl(PromiseImpl<OtherContinuation> rhs) noexcept(
        std::is_nothrow_constructible_v<Continuation, OtherContinuation&&>)
        : cont_(rhs.cont_.has_value()
                    ? Some(Continuation(std::move(rhs.cont_.value())))
                    : None) {}

    /// Discards the promise's continuation, leaving it empty.
    constexpr PromiseImpl& operator=(std::nullptr_t) noexcept {
        cont_.clear();
        return *this;
    }

    /// Assigns the promise's continuation
    constexpr PromiseImpl& operator=(Continuation continuation) noexcept(
        std::is_nothrow_move_constructible_v<Continuation>) {
        cont_.assign(std::move(continuation));
        return *this;
    }

    /// Destroys the promise, releasing its continuation
    ~PromiseImpl() = default;

    /// Returns true if the promise is non-empty (has a valid continuation).
    constexpr explicit operator bool() const noexcept {
        return cont_.has_value();
    }

    /// Invokes the promise's continuation.
    ///
    /// This method should be called by an executor to evaluate the promise.
    /// If the result's state is unresolved then the executor is responsible
    /// for arranging to invoke the promise's continuation again once it
    /// determines that it is possible to make progress towards completion of
    /// the promise encapsulated within the promise.
    ///
    /// Once the continuation returns a ready result, the promise is assigned
    /// an empty continuation.
    ///
    /// Throws `OptionEmptyException` when the promise is empty.
    constexpr result_type operator()(Context& ctx) {
        result_type result = (cont_.value())(ctx);
        if (!result.is_pending()) {
            cont_.clear();
        }
        return result;
    }

    /// Takes the promise's continuation, leaving it in an empty state.
    ///
    /// Throws `OptionEmptyException` when the promise is empty.
    constexpr Continuation take_continuation() {
        auto continuation = std::move(cont_.value());
        cont_.clear();
        return continuation;
    }

    /// Returns an unboxed promise which invokes the specified handler function
    /// after this promise completes (successfully or unsuccessfully), passing
    /// its result.
    ///
    /// The received result's state is guaranteed to be either `AsyncOk` or
    /// `AsyncError`, never `AsyncPending`.
    ///
    /// `handler` is a callable object (such as a lambda) which consumes the
    /// result of this promise and returns a new result with any value type
    /// and error type.
    ///
    /// The handler must return one of the following types:
    /// - AsyncResult<new_value_type, new_error_type>
    /// - AsyncOk<new_value_type>
    /// - AsyncError<new_error_type>
    /// - AsyncPending
    /// - Promise<new_value_type, new_error_type>
    /// - any callable or unboxed promise with the following signature:
    ///   AsyncResult<new_value_type, new_error_type>(Context&)
    ///
    /// The handler must accept one of the following argument lists:
    /// - (result_type&)
    /// - (const result_type&)
    /// - (Context&, result_type&)
    /// - (Context&, const result_type&)
    ///
    /// Asserts that the promise is non-empty.
    /// This method consumes the promise's continuation, leaving it empty.
    ///
    /// # Examples
    ///
    /// ```
    /// auto f = make_promise(...)
    ///     .then([](AsyncResult<int, std::string>& result)
    ///              -> AsyncResult<std::string, Void> {
    ///         if (result.is_ok()) {
    ///             printf("received value: %d\n", result.value());
    ///             if (result.value() % 15 == 0) {
    ///                 return AsyncOk{"fizzbuzz"s};
    ///             } else if (result.value() % 3 == 0) {
    ///                 return AsyncOk{"fizz"s};
    ///             } else if (result.value() % 5 == 0) {
    ///                 return AsyncOk{"buzz"s};
    ///             }
    ///             return AsyncOk{std::to_string(result.value())};
    ///         } else {
    ///             printf("received error: %s\n", result.error().c_str());
    ///             return AsyncError{Void{}};
    ///         }
    ///     })
    ///     .then(...);
    /// ```
    template <typename ResultHandler>
    constexpr auto then(ResultHandler handler) {
        static_assert(is_functor_v<ResultHandler>,
                      "ResultHandler must be callable");

        assert(cont_.has_value());
        return with_continuation(
            internal::ThenContinuation<PromiseImpl, ResultHandler>(
                std::move(*this), std::move(handler)));
    }

    /// Returns an unboxed promise which invokes the specified handler Function
    /// after this promise completes successfully, passing its resulting value.
    ///
    /// `handler` is a callable object (such as lambda) which consumes the
    /// result of this promise and returns a new result with any value type
    /// but the same error type.
    ///
    /// The handler must return one of the following types:
    /// - AsyncResult<new_value_type, error_type>
    /// - AsyncOk<new_value_type>
    /// - AsyncError<error_type>
    /// - AsyncPending
    /// - Promise<new_value_type, error_type>
    /// - any callable or unboxed promise with the following signature:
    ///   AsyncResult<new_value_type, error_type>(Context&)
    ///
    /// The handler must accept one of the following argument lists:
    /// - (value_type&)
    /// - (const value_type&)
    /// - (Context&, value_type&)
    /// - (Context&, const value_type&)
    ///
    /// Asserts that the promise is non-empty.
    /// This method consumes the promise's continuation, leaving it empty.
    ///
    /// # Examples
    ///
    /// ```
    /// auto f = make_promise(...)
    ///     .and_then([](const int& value) {
    ///         printf("received value: %d\n", value);
    ///         if (value % 15 == 0) {
    ///             return AsyncOk{"fizzbuzz"s};
    ///         } else if (value % 3 == 0) {
    ///             return AsyncOk{"fizz"s};
    ///         } else if (value % 5 == 0) {
    ///             return AsyncOk{"buzz"s};
    ///         }
    ///         return AsyncOk{std::to_string(value)};
    ///     })
    ///     .then(...);
    /// ```
    template <typename ValueHandler>
    constexpr auto and_then(ValueHandler handler) {
        static_assert(is_functor_v<ValueHandler>,
                      "ValueHandler must be callable");

        assert(cont_.has_value());
        return with_continuation(
            internal::AndThenContinuation<PromiseImpl, ValueHandler>(
                std::move(*this), std::move(handler)));
    }

    /// Returns an unboxed promise which invokes the specified handler function
    /// after this promise completes with an error, passing its resulting error.
    ///
    /// `handler` is a callable object (such as a lambda) which consumes the
    /// result of this promise and returns a new result with any error type
    /// but the same value type.
    ///
    /// The handler must returns one of the following types:
    /// - AsyncResult<value_type, new_error_type>
    /// - AsyncOk<value_type>
    /// - AsyncError<new_error_type>
    /// - AsyncPending
    /// - Promise<value_type, new_error_type>
    /// - any callable or unboxed promise with the following signature:
    ///   AsyncResult<value_type, new_error_type>(Context&)
    ///
    /// The handler must accept one of the following argument lists:
    /// - (error_type&)
    /// - (const error_type&)
    /// - (Context&, error_type&)
    /// - (Context&, const error_type&)
    ///
    /// Asserts that the promise is non-empty.
    /// This method consumes the promise's continuation, leaving it empty.
    ///
    /// # Examples
    ///
    /// ```
    /// auto f = make_promise(...)
    ///     .or_else([](const std::string& error) -> AsyncResult<int, Void> {
    ///         printf("received error: %s\n", error.c_str());
    ///         return AsyncError{Void{}};
    ///     })
    ///     .then(...);
    /// ```
    template <typename ErrorHandler>
    constexpr auto or_else(ErrorHandler handler) {
        static_assert(is_functor_v<ErrorHandler>,
                      "ErrorHandler must be callable");

        assert(cont_.has_value());
        return with_continuation(
            internal::OrElseContinuation<PromiseImpl, ErrorHandler>(
                std::move(*this), std::move(handler)));
    }

    /// Returns an unboxed promise which invokes the specified handler function
    /// after this promise completes (successfully or unsuccessfully), passing
    /// it the promise's result then delivering the result onwards to the next
    /// promise once the handler returns.
    ///
    /// The handler receives a const reference, or non-const reference
    /// depending on the signature of the handler's last argument.
    ///
    /// - const reference are especially useful for inspecting a result
    ///   mid-stream without modification, such as printing it for debugging.
    /// - non-const reference are especially useful for synchronously modifying
    ///   a result mid-stream, such as clamping its bounds or inject a default
    ///   value.
    ///
    /// `handler` is a callable object (such as a lambda) which can examine
    /// or modify the incoming result. Unlike `then()`, the handler does not
    /// need to propagate the result onwards.
    ///
    /// The handler must accept one of the following argument lists:
    /// - (result_type&)
    /// - (const result_type&)
    /// - (Context&, result_type&)
    /// - (Context&, const result_type&)
    ///
    /// Asserts that the promise is non-empty.
    /// This method consumes the promise's continuation, leaving it empty.
    ///
    /// # Examples
    ///
    /// ```
    /// auto f = make_promise(...)
    ///     .inspect([](const AsyncResult<int, std::string>& result) {
    ///         if (result.is_ok()) {
    ///             printf("received value: %d\n", result.value());
    ///         } else {
    ///             printf("received error: %s\n", result.error().c_str());
    ///         }
    ///     })
    ///     .then(...);
    /// ```
    template <typename InspectHandler>
    constexpr auto inspect(InspectHandler handler) {
        static_assert(is_functor_v<InspectHandler>,
                      "InspectHandler must be callable");
        static_assert(
            std::is_void_v<
                boost::callable_traits::return_type_t<InspectHandler>>,
            "InspectHandler must return void");

        assert(cont_.has_value());
        return with_continuation(
            internal::InspectContinuation<PromiseImpl, InspectHandler>(
                std::move(*this), std::move(handler)));
    }

    /// Returns an unboxed promise which discards the result of this promise
    /// once it completes, thereby always producing a successful result of type
    /// `AsyncResult<Void, Void>` regardless of whether this promise succeed or
    /// failed.
    ///
    /// Asserts that the promise is non-empty.
    /// This method consumes the promise's continuation, leaving it empty.
    ///
    /// # Examples
    ///
    /// ```
    /// auto f = make_promise(...)
    ///     .discard_result()
    ///     .then(...);
    /// ```
    constexpr auto discard_result() {
        assert(cont_.has_value());
        return with_continuation(
            internal::DiscardResultContinuation<PromiseImpl>(std::move(*this)));
    }

    // TODO document
    /// Applies a `wrapper` to the promise. Invokes the wrapper's `wrap()`
    /// method, passes the promise to the wrapper by value followed by any
    /// additional `args` passed to `wrap_with()`, then returns the wrapper's
    /// result.
    ///
    /// `Wrapper` is a type that implements a method called `wrap()` which
    /// accepts a promise as its argument and produces a wrapped result of any
    /// type, such as another promise.
    ///
    /// Asserts that the promise is non-empty.
    /// This method consumes the promise's continuation, leaving it empty.
    ///
    /// # Examples
    ///
    /// In the example, `Sequencer` is a wrapper type that imposes FIFO
    /// execution order onto a sequence of wrapper promise.
    ///
    ///
    template <typename Wrapper, typename... Args>
    constexpr auto wrap_with(Wrapper& wrapper, Args&&... args) {
        assert(cont_.has_value());
        return wrapper.wrap(std::move(*this), std::forward<Args>(args)...);
    }

    /// Wraps the promise's continuation into a `Function`
    ///
    /// A Boxed promise is easier to store and pass around than the unboxed
    /// promises produced by `make_promise` and combinators, though
    /// boxing may incur a heap allocation.
    ///
    /// It's a good idea to defer boxing the promise until after all desired
    /// combinators have been applied to prevent unnecessary heap allocation
    /// during intermediate states of the promise's construction.
    ///
    /// Returns an empty promise if the promise is empty.
    /// This method consumes the promise's continuation, leaving it empty.
    ///
    /// # Examples
    ///
    /// ```
    /// // f's type is a `Promise<Continuation>` whose continuation contains
    /// // an anonymous type (the lambda)
    /// auto f = make_promise([](Context&) -> Result<Void, Void> { ... });
    ///
    /// // g's type will be `Promise<Function<Result<T, E>(Context&)>>` due to
    /// // boxing
    /// auto boxed_f = f.box();
    ///
    /// // Alternately, we can get exactly the same effect by assigning the
    /// // unboxed promise to a variable of a named type instead of calling
    /// // `box()`
    /// Promise<Void, Void> boxed_f = std::move(f);
    /// ```
    constexpr PromiseImpl<Function<result_type(Context&)>> box() {
        return std::move(*this);
    }

    /// Swaps the promise's continuation.
    constexpr void
    swap(PromiseImpl& rhs) noexcept(std::is_nothrow_swappable_v<Continuation>) {
        using std::swap;
        swap(cont_, rhs.cont_);
    }

private:
    Option<Continuation> cont_;
};

/// Swaps the `Promise`
template <typename Continuation>
constexpr void
swap(PromiseImpl<Continuation>& lhs, PromiseImpl<Continuation>& rhs) noexcept(
    std::is_nothrow_swappable_v<Continuation>) {
    lhs.swap(rhs);
}

/// Compares with `nullptr`
template <typename Continuation>
constexpr bool operator==(const PromiseImpl<Continuation>& p,
                          std::nullptr_t) noexcept {
    return !p;
}

template <typename Continuation>
constexpr bool operator==(std::nullptr_t,
                          const PromiseImpl<Continuation>& p) noexcept {
    return !p;
}

template <typename Continuation>
constexpr bool operator!=(const PromiseImpl<Continuation>& p,
                          std::nullptr_t) noexcept {
    return !!p;
}

template <typename Continuation>
constexpr bool operator!=(std::nullptr_t,
                          const PromiseImpl<Continuation>& p) noexcept {
    return !!p;
}

/// make_promise
///
/// Returns an unboxed promise that wraps the specified handler.
/// The type of the promise's result is inferred from the handler's result.
///
/// `handler` is a callable object (such as a lambda).
///
/// The handler must return one of the following types:
/// - AsyncResult<T, E>
/// - AsyncOk<T>
/// - AsyncError<E>
/// - AsyncPending
/// - Promise<T, E>
/// - any callable or unboxed promise with the following signature:
///   `AsyncResult<T, E>(Context&)`
///
/// The handler must accept one of the following argument lists:
/// - ()
/// - (Context&)
///
/// See documentation of `Promise` for more information.
///
/// # Examples
///
/// ```
/// enum class WeatherType {
///     SUNNY,
///     GLORIOUS,
///     CLOUDY,
///     EERIE,
/// };
///
/// WeatherType look_outside() { ... }
///
/// void wait_for_tomorrow(SuspendedTask task) {
///     // arrange to call task.resume_task() tomorrow
/// }
///
/// Promise<WeatherType, std::string> wait_for_good_weather(int max_days) {
///     return make_promise([days_left = max_days](Context& ctx) mutable
///                     -> AsyncResult<WeatherType, std::string> {
///         WeatherType weather = look_outside();
///         if ((weather == WeatherType::SUNNY) ||
///             (weather == WeatherType::GLORIOUS)) {
///             return AsyncOk(weather);
///         }
///
///         if (days_left > 0) {
///             wait_for_tomorrow(ctx.suspend_task());
///             return AsyncPending{};
///         }
///         --days_left;
///         return AsyncError("nothing but grey skies"s);
///     });
/// }
///
/// auto f = wait_for_good_weather(7)
///     .and_then([](const WeatherType& weather) { ... })
///     .or_else([](const std::string& error) { ... });
/// ```
template <typename PromiseHandler>
constexpr auto make_promise(PromiseHandler handler) {
    static_assert(is_functor_v<PromiseHandler>,
                  "PromiseHandler must be callable");

    return PromiseImpl(
        internal::PromiseContinuation<PromiseHandler>(std::move(handler)));
}

/// make_result_promise
///
/// Returns an unboxed promise that immediately returns the specified result
/// when invoked.
///
/// This function is especially useful for returning promise from functions
/// that have multiple branches some of which complete synchronously.
template <typename T, typename E>
constexpr auto make_result_promise(AsyncResult<T, E>&& result) {
    return PromiseImpl(internal::ResultContinuation<T, E>(std::move(result)));
}

template <typename T, typename E>
constexpr auto make_result_promise(const AsyncResult<T, E>& result) {
    return PromiseImpl(internal::ResultContinuation<T, E>(result));
}

/// make_ok_promise
///
/// Returns an unboxed promise that immediately returns the specified value
/// when invoked.
///
/// This function is especially useful for returning promises from functions
/// that have multiple branches some of which complete synchronously.
template <typename T, typename E>
constexpr auto make_ok_promise(T&& value) {
    return make_result_promise<T, E>(AsyncOk(std::move(value)));
}

template <typename T, typename E>
constexpr auto make_ok_promise(const T& value) {
    return make_result_promise<T, E>(AsyncOk(value));
}

/// make_error_promise
///
/// Returns an unboxed promise that immediately returns the specified error
/// when invoked.
///
/// This function is especially useful for returning promises from functions
/// that have multiple branches some of which complete synchronously.
template <typename T, typename E>
constexpr auto make_error_promise(E&& error) {
    return make_result_promise<T, E>(AsyncError(std::move(error)));
}

template <typename T, typename E>
constexpr auto make_error_promise(const E& error) {
    return make_result_promise<T, E>(AsyncError(error));
}

/// join_promises
///
/// Jointly evaluates zero or more promises.
/// Returns a promise that produces a `std::tuple` containing the result
/// of each promise once they all complete.
///
/// # Examples
///
/// ```
/// auto get_random_number() {
///     return make_ok_promise<int, int>(random() % 10);
/// }
///
/// auto get_random_product() {
///     auto f = get_random_number();
///     auto g = get_random_number();
///     return join_promises(std::move(f), std::move(g))
///         .and_then([](std::tuple<AsyncResult<int, int>,
///                                 AsyncResult<int, int>>& results) {
///             auto& [r0, r1] = results;
///             return AsyncOk(r0.value() + r1.value);
///         });
/// }
/// ```
template <typename... Promises>
constexpr auto join_promises(Promises... promises) {
    return PromiseImpl(
        internal::JoinContinuation<Promises...>(std::move(promises)...));
}

/// join_promise_vector
///
/// Jointly evaluates zero or more homogenous promises (same result type).
/// Returns a promise that produces a `std::vector` containing the result
/// of each promise once the all complete.
///
/// # Examples
///
/// ```
/// auto get_random_number() {
///     return make_o_promise<int, int>(random() % 10);
/// }
///
/// auto get_random_product() {
///     std::vector<Promise<int, int>> promises;
///     promises.push_back(get_random_number());
///     promises.push_back(get_random_number());
///     return join_promise_vector(std::move(promises))
///         .and_then([](std::vector<AsyncResult<int, int>>& results) {
///             return AsyncOk(results[0].value() + results[1].value());
///         });
/// }
/// ```
template <typename T, typename E>
constexpr auto join_promise_vector(std::vector<Promise<T, E>> promises) {
    return PromiseImpl(
        internal::JoinVectorContinuation<Promise<T, E>>(std::move(promises)));
}

// Makes a promise containing the specified continuation.
//
// NOTE: Internal use only.
template <typename Continuation>
constexpr PromiseImpl<Continuation> with_continuation(Continuation c) {
    return PromiseImpl<Continuation>(std::move(c));
}

} // namespace bipolar

#include "bipolar/futures/future_inl.hpp"

#endif
