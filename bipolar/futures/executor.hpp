//! The Executor base class
//!

#ifndef BIPOLAR_FUTURES_EXECUTOR_HPP_
#define BIPOLAR_FUTURES_EXECUTOR_HPP_

#include "bipolar/futures/pending_task.hpp"

#include <functional>

namespace bipolar {
/// Executor
///
/// # Brief
///
/// An abstract interface for executing asynchronous tasks, such as promises,
/// represented by `PendingTask`.
///
/// # Execution
///
/// An executor evaluates its task incrementally. During each iteration fo the
/// executor's main loop, it invokes the next task from its ready queue.
///
/// If the task returns true, then the task is deemed to have completed. The
/// executor removes the tasks from its queue and destroys it since there is
/// nothing left to do.
///
/// If the task returns false, then the task is deemed to have voluntarily
/// suspended itself pending some events that is awaiting. Prior to returning,
/// the task should acquire at least on `SuspendedTask` handle from its
/// execution context using `Context::suspend_task()` to provide a means for
/// the task to be resumed once it can make forward progress again.
///
/// Once the suspended task is resumed with `SuspendedTask::resume()`, it is
/// moved back to the ready queue and it will be invoked again during a later
/// iteration of the executor's loop.
///
/// If all `SuspendedTask` handles for a given task are destroyed without the
/// task ever being resumed then the task is also destryed since there would be
/// no way for the task to be resumed from suspension. We say that such a task
/// has been "abandoned".
///
/// The executor retains single-ownership of all active and suspended tasks.
/// When the executor is destroyed, all of its remaining tasks are also
/// destryed.
///
/// Please read `Promise` for a more detailed explanation of the
/// responsibilities of tasks and executors.
///
/// # Note
///
/// This interface is designed to support a variety of different executor
/// implementations. For example, one implementation might run its tasks on
/// a single thread whereas another might dispatch them on an event-driven
/// message loop or use a thread pool.
///
/// See also `SingleThreadedExecutor` for a concrete implementation.
class Executor {
public:
    /// Destroys the executor along with all of its remaining scheduled tasks
    /// that have yet to complete
    virtual ~Executor() = default;

    /// Schedules a task for eventual execution by the executor
    ///
    /// This method is thread-safe
    virtual void schedule_task(PendingTask task) = 0;
};
} // namespace bipolar

#endif
