//! Scheduler
//!
//! See documentation of `Scheduler` for more information.

#ifndef BIPOLAR_FUTURES_SCHEDULER_HPP_
#define BIPOLAR_FUTURES_SCHEDULER_HPP_

#include <cstdint>
#include <map>
#include <queue>

#include "bipolar/futures/pending_task.hpp"
#include "bipolar/futures/promise.hpp"
#include "bipolar/futures/suspended_task.hpp"

#include <boost/noncopyable.hpp>

namespace bipolar {
/// Scheduler
///
/// Keeps track of runnable and suspended tasks.
/// This is low-level building block for implementing executors.
/// For a concrete implementation, see `SingleThreadedExecutor`.
///
/// Instance of this object are not thread-safe. Its client is responsible
/// for providing all necessary synchronization.
class Scheduler final : public boost::noncopyable {
public:
    using TaskQueue = std::queue<PendingTask>;

    Scheduler() = default;
    ~Scheduler() = default;

    /// Adds a task to the runnable queue.
    ///
    /// Preconditions:
    /// - `task` must tbe non-empty
    void schedule_task(PendingTask task);

    /// Obtains a new ticket win a ref-count of `initial_refs`.
    /// The executor must eventually call `finalize_ticket()` to update the
    /// state of the ticket.
    ///
    /// Preconditions:
    /// - `initial_refs` must be at least 1
    SuspendedTask::Ticket obtain_ticket(std::uint32_t initial_refs = 1);

    /// Updates a ticket after one run of a task's continuation according
    /// to the state of the task after its run. The executor must call this
    /// method after calling `obtain_ticket()` to indicate the disposition of
    /// the task for which the ticket was obtained.
    ///
    /// Passing an empty `task` indicates that the task has completed so it
    /// does not need to be resumed.
    ///
    /// Passing a non-empty `task` indicates that the task returned a pending
    /// result and may need to be suspended depending on the current state
    /// of the ticket.
    /// - If the ticket has already been resumed, moves `task` into the
    ///   runnable queue.
    /// - Otherwise, if the ticket still ahas a non-zero ref-count, moves `task`
    ///   into the suspended task table.
    /// - Otherwise, considers the task abandoned and the caller retains
    ///   ownership of `task`
    ///
    /// Preconditions:
    /// - `task` must be non-empty (may be empty)
    /// - the ticket must not have already been finalized
    void finalize_ticket(SuspendedTask::Ticket ticket, PendingTask* task);

    /// Increases the ticket's ref-count.
    ///
    /// Preconditions:
    /// - the ticket's ref-count must be non-zero (positive)
    void duplicate_ticket(SuspendedTask::Ticket ticket);

    /// Decreases the ticket's ref-count.
    ///
    /// If the task's ref-count reaches 0 and has an associated task that
    /// has not already been resumed, returns the associated task back
    /// to the caller.
    /// Otherwise, returns an empty task.
    ///
    /// Preconditions:
    /// - the ticket's ref-count must be non-zero (positive)
    PendingTask release_ticket(SuspendedTask::Ticket ticket);

    /// Resumes a task and decreases the ticket's ref-count.
    ///
    /// If the ticket has an associated task that has not already been resumed,
    /// moves its associated task to the runnable queue and returns true.
    /// Otherwise, returns false.
    ///
    /// Preconditions:
    /// - the ticket's ref-count must be non-zero (positive)
    bool resume_task_with_ticket(SuspendedTask::Ticket ticket);

    /// Takes all tasks in the runnable queue.
    ///
    /// Preconditions:
    /// - `tasks` must be non-null and empty
    void take_runnable_tasks(TaskQueue* tasks);

    /// Takes all remaining tasks, regardless of whether they are runnable
    /// or suspended.
    ///
    /// This operation is useful when shutting donw an executor.
    ///
    /// Preconditions:
    /// - `tasks` must be non-null and empty
    void take_all_tasks(TaskQueue* tasks);

    /// Returns true if there are any runnable tasks.
    bool has_runnable_tasks() const noexcept {
        return !runnable_tasks_.empty();
    }

    /// Returns true if there are any suspended tasks that have yet to
    /// be resumed.
    bool has_suspended_tasks() const noexcept {
        return suspended_task_count_ > 0;
    }

    /// Returns true if there are any tickets that have yet to be finalized.
    bool has_outstanding_tickets() const noexcept {
        return !tickets_.empty();
    }

private:
    struct TicketRecord {
        TicketRecord(std::uint32_t initial_refs) noexcept
            : ref_count(initial_refs), was_resumed(false) {}

        /// The current reference count
        std::uint32_t ref_count;

        /// True if the task has been resumed using `resume_task_with_ticket()`
        bool was_resumed;

        /// The task is initially empty when the ticket is obtained.
        /// It is later set to non-empty if the task needs to be suspended when
        /// the ticket is finalized. It becomes empty again when the task
        /// is moved into the runnable queue, released, or taked.
        PendingTask task;
    };

    TaskQueue runnable_tasks_;
    std::map<SuspendedTask::Ticket, TicketRecord> tickets_;
    std::uint64_t suspended_task_count_ = 0;
    SuspendedTask::Ticket next_ticket_ = 1;
};

} // namespace bipolar

#endif
