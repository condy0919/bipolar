//!
//!
//!

#ifndef BIPOLAR_FUTURES_CONTEXT_HPP_
#define BIPOLAR_FUTURES_CONTEXT_HPP_

#include <type_traits>

#include "bipolar/futures/suspended_task.hpp"

namespace bipolar {
// forward
class Executor;

/// Context
///
/// Execution context for an asynchronous task, such as a `Promise`, `Future`
/// or `PendingTask`.
///
/// When an `Executor` executes a task, it provides the task with an execution
/// context which enables the task to communicate with the executor and manage
/// its own lifecycle. Specialized executors may subclass `Context` and offer
/// additional methods beyond those which are defined here, such as to provides
/// access to platform-specific features supported by the executor.
///
/// The context provided to a task is only valid within the scope of a single
/// invocation; the task must not retain a reference to the context accross
/// invocation.
///
/// See documentation of `Promise` for more information.
class Context {
public:
    /// Gets the `Executor` that is running the task, never null.
    virtual Executor* get_executor() const = 0;

    /// Obtains a handle that can be used to resume the task after it has been
    /// suspended.
    ///
    /// Clients should call this method before returning `pending()` from the
    /// task.
    ///
    /// See documentation of `Executor` for more information.
    virtual SuspendedTask suspend_task() = 0;

    /// Converts this `Context` to a derived context type.
    template <typename Derived,
              std::enable_if_t<std::is_base_of_v<Context, Derived>, int> = 0>
    Derived& as() & {
        // TODO: benchmark dynamic_cast
        // TODO: runtime-check
        return static_cast<Derived&>(*this);
    }

protected:
    virtual ~Context() = default;
};
} // namespace bipolar

#endif
