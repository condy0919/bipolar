//! SuspendedTask
//!
//! See `SuspendedTask` for details.
//!

#ifndef BIPOLAR_FUTURES_SUSPENDED_TASK_HPP_
#define BIPOLAR_FUTURES_SUSPENDED_TASK_HPP_

#include <cstdint>
#include <utility>

namespace bipolar {
/// SuspendedTask
///
/// Represents a task that is awaiting resumption.
///
/// This object has RAII semantics. If the task is not resumed by at least
/// on holder of it's `SuspendedTask` handles, then it will be destroyed
/// by the executor since it is no longer possible for the task to make
/// progress. The task is said have been "abandoned".
///
/// See documentation of `Executor` for more information.
class SuspendedTask final {
public:
    /// A handle that grants the capability to resume a `SuspendedTask`
    /// Each issued ticked must be individually resolved.
    using Ticket = std::uint64_t;

    /// The resolver mechanism implements a lightweight form of reference
    /// counting for tasks that have been suspended.
    ///
    /// When a `SuspendedTask` is created in a non-empty state, it receives
    /// a pointer to a resolver interface and a ticket. The ticket is
    /// a one-time-use handle that represents the task that was suspended
    /// and provides a means to resume it. The `SuspendedTask` class ensures
    /// that every ticket is precisely accounted for.
    ///
    /// When `SuspendedTask::resume_task()` is called on an instance with
    /// a valid ticket, the resolver's `resolve_ticket()` method is invoked
    /// passing the ticket's value along with *true* to resume the task. This
    /// operation consumes the ticket so the `SuspendedTask` transitions to
    /// an empty state. The ticket and resolver cannot be used again by this
    /// `SuspendedTask` instance.
    ///
    /// Similarly, when `SuspendedTask::reset()` is called on an instance with
    /// a valid ticket or when the task goes out of scope on such an instance,
    /// the resolver's `resolve_ticket()` method is invoked but this time passes
    /// *false* to not resume the task. As before, the ticket is consumed.
    ///
    /// Finally, when the `SuspendedTask` is copied, its ticket is duplicated
    /// using `duplicate_ticket()` resulting in two tickets, both of which must
    /// be individually resolved.
    ///
    /// Resuming a task that has already been resumed has no effect.
    /// Conversely, a task is considered "abandoned" if all of its tickets
    /// have been resolved without it ever being resumed. See documentation
    /// of `Promise` for more information.
    ///
    /// The method of `Resolver` class are safe to call from any thread,
    /// including threads that may not be managed by the task's executor.
    class Resolver {
    public:
        /// Duplicates the provided ticket, returning a new ticket.
        ///
        /// NOTE:
        /// the new ticket may have the same numeric value as the original
        /// ticket but should be considered a distinct instance that must be
        /// separatedly resolved.
        virtual Ticket duplicate_ticket(Ticket ticket) = 0;

        /// Consumes the provided ticket, optionally resuming its
        /// associated task. The provided ticket must not be used again.
        virtual void resolve_ticket(Ticket ticket, bool resume_task) = 0;

    protected:
        virtual ~Resolver() = default;
    };

    constexpr SuspendedTask() noexcept : resolver_(nullptr), ticket_(0) {}

    constexpr SuspendedTask(Resolver* resolver, Ticket ticket) noexcept
        : resolver_(resolver), ticket_(ticket) {}

    constexpr SuspendedTask(const SuspendedTask& rhs) noexcept
        : resolver_(rhs.resolver_),
          ticket_(resolver_ ? resolver_->duplicate_ticket(rhs.ticket_) : 0) {}

    constexpr SuspendedTask(SuspendedTask&& rhs) noexcept
        : resolver_(rhs.resolver_), ticket_(rhs.ticket_) {
        rhs.resolver_ = nullptr;
    }

    /// Releases the task without resumption.
    ///
    /// Does nothing if this object does not hold a ticket
    ~SuspendedTask() {
        reset();
    }

    constexpr SuspendedTask& operator=(const SuspendedTask& rhs) noexcept {
        if (this != &rhs) {
            reset();
            resolver_ = rhs.resolver_;
            ticket_ = resolver_ ? resolver_->duplicate_ticket(rhs.ticket_) : 0;
        }
        return *this;
    }

    constexpr SuspendedTask& operator=(SuspendedTask&& rhs) noexcept {
        if (this != &rhs) {
            reset();
            resolver_ = rhs.resolver_;
            ticket_ = rhs.ticket_;
            rhs.resolver_ = nullptr;
        }
        return *this;
    }

    /// Returns true if this object holds a ticket for a suspended task.
    constexpr explicit operator bool() const noexcept {
        return resolver_ != nullptr;
    }

    /// Asks the task's executor to resume execution of the `SuspendedTask`
    /// if it has not already been resumed or completed. Also releases
    /// the task's ticket as a side-effect.
    ///
    /// Clients should call this method when i is possible for the task to
    /// make progress; for example, because some event the task as awaiting
    /// has occurred. See documentation of `Executor` for more information.
    ///
    /// Does nothing if this object does not hold a ticket.
    constexpr void resume_task() {
        resolve(true);
    }

    /// Releases the `SuspendedTask` without resumption.
    ///
    /// Does nothing if this object does not hold a ticket.
    constexpr void reset() {
        resolve(false);
    }

    /// Swaps `SuspendedTask`
    /// NOTE: `std::swap` is `constexpr` since C++20
    /*constexpr*/ void swap(SuspendedTask& rhs) noexcept {
        std::swap(resolver_, rhs.resolver_);
        std::swap(ticket_, rhs.ticket_);
    }

private:
    /// Makes it `constexpr` to be ready to C++20
    constexpr void resolve(bool resume_task) {
        if (resolver_) {
            // Move the ticket to the stack to guard against possible
            // re-entrance occurring as a side-effect of the task's own
            // destructor running.
            Resolver* tmp_resolver = resolver_;
            Ticket tmp_ticket = ticket_;
            resolver_ = nullptr;
            tmp_resolver->resolve_ticket(tmp_ticket, resume_task);
        }
    }

    Resolver* resolver_;
    Ticket ticket_;
};

/// Swaps the `SuspendedTask`
inline /*constexpr*/ void swap(SuspendedTask& lhs,
                               SuspendedTask& rhs) noexcept {
    lhs.swap(rhs);
}
} // namespace bipolar

#endif
