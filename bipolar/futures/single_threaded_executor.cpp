#include "bipolar/futures/single_threaded_executor.hpp"

#include <cassert>
#include <condition_variable>
#include <mutex>

#include "bipolar/core/thread_safety.hpp"

namespace bipolar {
// The dispatcher runs tasks and provides the suspended task resolver.
//
// The lifetime of this object is somewhat complex since there are pointers
// to it from multiple sources which are released in different ways.
//
// - `SingleThreadedExecutor` holds a pointer in `dispatcher_` which it
//   releases after calling `shutdown()` to inform the dispatcher of its
//   own demise
// - `SuspendedTask` holds a pointer to the dispatcher's resolver interface
//   and the number of outstanding pointers corresponds to the number of
//   outstanding suspended task tickets tracked by `scheduler_`
class SingleThreadedExecutor::DispatcherImpl : public SuspendedTask::Resolver {
public:
    DispatcherImpl() = default;

    ~DispatcherImpl() {
        std::lock_guard lock(mtx_);
        assert(was_shutdown_);
        assert(!scheduler_.has_runnable_tasks());
        assert(!scheduler_.has_suspended_tasks());
        assert(!scheduler_.has_outstanding_tickets());
    }

    void shutdown() {
        Scheduler::TaskQueue tasks;
        {
            std::lock_guard lock(mtx_);
            assert(!was_shutdown_);
            was_shutdown_ = true;
            scheduler_.take_all_tasks(&tasks);
            if (scheduler_.has_outstanding_tickets()) {
                // cannot delete self yet
                return;
            }
        }

        delete this;
    }

    void schedule_task(PendingTask task) {
        {
            std::lock_guard lock(mtx_);
            assert(!was_shutdown_);
            scheduler_.schedule_task(std::move(task));
            if (!need_wake_) {
                // don't need to wake
                return;
            }
            need_wake_ = false;
        }

        // It's more efficient to notify outside the lock
        wake_.notify_one();
    }

    void run(ContextImpl& ctx) {
        Scheduler::TaskQueue tasks;
        while (true) {
            wait_for_runnable_tasks(&tasks);
            if (tasks.empty()) {
                return;
            }

            do {
                run_task(&tasks.front(), ctx);
                tasks.pop(); // the task may be destroyed here if it's not
                             // suspended
            } while (!tasks.empty());
        }
    }

    // Must only be called while `run_task()` is running a task.
    // This happens when the task's continuation calls `Context::suspend_task`
    // upon the context it received as an argument.
    SuspendedTask suspend_current_task() {
        std::lock_guard lock(mtx_);
        assert(!was_shutdown_);
        if (current_task_ticket_ == 0) {
            current_task_ticket_ =
                scheduler_.obtain_ticket(/*initial_refs = */ 2);
        } else {
            scheduler_.duplicate_ticket(current_task_ticket_);
        }
        return SuspendedTask(this, current_task_ticket_);
    }

    SuspendedTask::Ticket
    duplicate_ticket(SuspendedTask::Ticket ticket) override {
        std::lock_guard lock(mtx_);
        scheduler_.duplicate_ticket(ticket);
        return ticket;
    }

    void resolve_ticket(SuspendedTask::Ticket ticket,
                        bool resume_task) override {
        PendingTask abandoned_task;
        bool do_wake = false;
        {
            std::lock_guard lock(mtx_);
            if (resume_task) {
                scheduler_.resume_task_with_ticket(ticket);
            } else {
                abandoned_task = scheduler_.release_ticket(ticket);
            }

            if (was_shutdown_) {
                assert(!need_wake_);
                if (scheduler_.has_outstanding_tickets()) {
                    // cannot shutdown yet
                    return;
                }
            } else if (need_wake_ && (scheduler_.has_runnable_tasks() ||
                                      !scheduler_.has_suspended_tasks())) {
                need_wake_ = false;
                do_wake = true;
            } else {
                // nothing else to do
                return;
            }
        }

        if (do_wake) {
            wake_.notify_one();
        } else {
            delete this;
        }
    }

private:
    void wait_for_runnable_tasks(Scheduler::TaskQueue* tasks)
        BIPOLAR_NO_THREAD_SAFETY_ANALYSIS {
        std::unique_lock lock(mtx_);
        while (true) {
            assert(!was_shutdown_);
            scheduler_.take_runnable_tasks(tasks);
            if (!tasks->empty()) {
                return;
            }
            if (!scheduler_.has_suspended_tasks()) {
                return;
            }
            need_wake_ = true;
            wake_.wait(lock);
            need_wake_ = false;
        }
    }

    void run_task(PendingTask* task, Context& ctx) {
        assert(current_task_ticket_ == 0);
        const bool finished = (*task)(ctx);
        assert(!*task == finished);
        (void)finished;
        if (current_task_ticket_ == 0) {
            // task was not suspended, no ticket was produced
            return;
        }

        std::lock_guard lock(mtx_);
        assert(!was_shutdown_);
        scheduler_.finalize_ticket(current_task_ticket_, task);
        current_task_ticket_ = 0;
    }

private:
    SuspendedTask::Ticket current_task_ticket_ = 0;
    std::condition_variable wake_;

    // A bunch of state that is guarded by a mutex
    std::mutex mtx_;
    bool was_shutdown_ BIPOLAR_GUARDED_BY(mtx_) = false;
    bool need_wake_ BIPOLAR_GUARDED_BY(mtx_) = false;
    Scheduler scheduler_ BIPOLAR_GUARDED_BY(mtx_);
};

// FIXME unique_ptr
SingleThreadedExecutor::SingleThreadedExecutor()
    : ctx_(this), dispatcher_(new DispatcherImpl()) {}

SingleThreadedExecutor::~SingleThreadedExecutor() {
    dispatcher_->shutdown();
}

void SingleThreadedExecutor::schedule_task(PendingTask task) {
    assert(task);
    dispatcher_->schedule_task(std::move(task));
}

void SingleThreadedExecutor::run() {
    dispatcher_->run(ctx_);
}

SuspendedTask SingleThreadedExecutor::ContextImpl::suspend_task() {
    return executor_->dispatcher_->suspend_current_task();
}

} // namespace bipolar
