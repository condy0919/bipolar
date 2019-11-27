//!
//!
//!

#ifndef BIPOLAR_FUTURES_SINGLE_THREADED_EXECUTOR_HPP_
#define BIPOLAR_FUTURES_SINGLE_THREADED_EXECUTOR_HPP_

#include "bipolar/futures/executor.hpp"
#include "bipolar/futures/promise.hpp"
#include "bipolar/futures/scheduler.hpp"

#include <memory>

#include <boost/noncopyable.hpp>

namespace bipolar {
/// A simple platform-independent single-threaded asynchronous task executor.
///
/// This implementation is designed for use when writing simple single-threaded
/// platform-independent applications. It may be less efficient or provide
/// fewer features than more specialized or platform-dependent executors.
///
/// See documentation of `Promise` for more information.
class SingleThreadedExecutor final : public Executor,
                                     public boost::noncopyable {
public:
    SingleThreadedExecutor();

    /// Destroys the executor along with all of its remaining scheduled tasks
    /// that have yet to complete
    ~SingleThreadedExecutor() override;

    /// Schedules a task for eventual execution by the executor.
    ///
    /// This method is thread-safe.
    void schedule_task(PendingTask task) override;

    /// Runs all scheduled tasks (including additional tasks scheduled while
    /// they run) until none remain.
    ///
    /// This method is thread-safe but must only be called on at most one
    /// thread at a time
    void run();

private:
    class DispatcherImpl;

    // The task context for tasks run by the executor
    class ContextImpl : public Context {
    public:
        ContextImpl(SingleThreadedExecutor* executor) : executor_(executor) {}

        ~ContextImpl() override = default;

        SingleThreadedExecutor* get_executor() const override {
            return executor_;
        }

        SuspendedTask suspend_task() override;

    private:
        SingleThreadedExecutor* const executor_;
    };

    ContextImpl ctx_;
    DispatcherImpl* const dispatcher_;
};

} // namespace bipolar

#endif
