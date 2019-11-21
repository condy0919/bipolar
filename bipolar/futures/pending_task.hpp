//! PendingTask
//!
//! See `PendingTask` for details.
//!

#ifndef BIPOLAR_FUTURES_PENDING_TASK_HPP_
#define BIPOLAR_FUTURES_PENDING_TASK_HPP_

#include <cstdint>
#include <utility>

#include "bipolar/core/void.hpp"
#include "bipolar/futures/context.hpp"
#include "bipolar/futures/promise.hpp"

namespace bipolar {
/// PendingTask
///
/// A pending task holds a `BoxedPromise` that can be scheduled to run on an
/// `Executor` using `Executor::schedule_task()`.
///
/// An `Executor` repeatedly invokes a pending task until it returns true,
/// indicating completion. The promise's result is discarded since it is not
/// meaningful to the executor. If you need to consume the result, use a
/// combinator such as `then()` to capture it prior to wrapping the promise
/// into a pending task.
///
/// See documentation of `Promise` for more information.
class PendingTask final {
public:
    /// The type of promise held by this task
    using promise_type = Promise<Void, Void>;

    /// Creates an empty pending task without a promise
    PendingTask() noexcept = default;

    /// Creates a pending task that wraps an already boxed promise that returns
    /// `AsyncResult<Void, Void>`
    explicit PendingTask(promise_type p) noexcept : promise_(std::move(p)) {}

    /// Creates a pending task that wraps any kind of promise, boxed or unboxed,
    /// regardless of its result type and with any context that is assignable
    /// from this task's context type
    template <typename Continuation>
    explicit PendingTask(PromiseImpl<Continuation> p) noexcept
        : promise_(p ? p.discard_result().box() : promise_type{}) {}

    PendingTask(PendingTask&&) noexcept = default;
    PendingTask& operator=(PendingTask&&) noexcept = default;

    /// `PendingTask` is move-only
    PendingTask(const PendingTask&) = delete;
    PendingTask& operator=(const PendingTask&) = delete;

    /// Destroys the pending task, releasing its promise
    ~PendingTask() = default;

    /// Returns true if the pending task is non-empty (has a valid promise)
    explicit operator bool() const noexcept {
        return static_cast<bool>(promise_);
    }

    /// Evaluates the pending task.
    ///
    /// If the task completes (returns a non-pending result), the task reverts
    /// to an empty state (because the promise it holds has reverted to an
    /// empty state) and returns true.
    ///
    /// It is an error to invoke this method if the pending task is empty.
    bool operator()(Context& ctx) {
        return !promise_(ctx).is_pending();
    }

    /// Extracts the pending task's promise
    promise_type take_promise() noexcept {
        return std::move(promise_);
    }

private:
    promise_type promise_;
};


} // namespace bipolar

#endif
