#include "bipolar/futures/scheduler.hpp"

#include <cassert>
#include <utility>

namespace bipolar {
void Scheduler::schedule_task(PendingTask task) {
    assert(task);
    runnable_tasks_.push(std::move(task));
}

SuspendedTask::Ticket Scheduler::obtain_ticket(std::uint32_t initial_refs) {
    assert(initial_refs >= 1);

    SuspendedTask::Ticket ticket = next_ticket_++;
    tickets_.emplace(ticket, TicketRecord(initial_refs));
    return ticket;
}

void Scheduler::finalize_ticket(SuspendedTask::Ticket ticket,
                                PendingTask* task) {
    auto iter = tickets_.find(ticket);
    assert(iter != tickets_.end());
    assert(!iter->second.task);
    assert(iter->second.ref_count > 0);
    assert(task);

    --iter->second.ref_count;
    if (!*task) {
        // task already finished
    } else if (iter->second.was_resumed) {
        // task immediately became runnable
        runnable_tasks_.push(std::move(*task));
    } else if (iter->second.ref_count > 0) {
        // task remains suspended
        iter->second.task = std::move(*task);
        ++suspended_task_count_;
    } else {
        // task was abandoned and caller retains ownership of it
    }

    if (iter->second.ref_count == 0) {
        tickets_.erase(iter);
    }
}

void Scheduler::duplicate_ticket(SuspendedTask::Ticket ticket) {
    auto iter = tickets_.find(ticket);
    assert(iter != tickets_.end());
    assert(iter->second.ref_count > 0);

    ++iter->second.ref_count;
}

PendingTask Scheduler::release_ticket(SuspendedTask::Ticket ticket) {
    auto iter = tickets_.find(ticket);
    assert(iter != tickets_.end());
    assert(iter->second.ref_count > 0);

    --iter->second.ref_count;
    if (iter->second.ref_count == 0) {
        PendingTask task(std::move(iter->second.task));
        if (task) {
            assert(suspended_task_count_ > 0);
            --suspended_task_count_;
        }
        tickets_.erase(iter);
        return task;
    }
    return PendingTask();
}

bool Scheduler::resume_task_with_ticket(SuspendedTask::Ticket ticket) {
    auto iter = tickets_.find(ticket);
    assert(iter != tickets_.end());
    assert(iter->second.ref_count > 0);

    bool did_resume = false;
    --iter->second.ref_count;
    if (!iter->second.was_resumed) {
        iter->second.was_resumed = true;
        if (iter->second.task) {
            did_resume = true;
            assert(suspended_task_count_ > 0);
            --suspended_task_count_;
            runnable_tasks_.push(std::move(iter->second.task));
        }
    }

    if (iter->second.ref_count == 0) {
        tickets_.erase(iter);
    }
    return did_resume;
}

void Scheduler::take_runnable_tasks(TaskQueue* tasks) {
    assert(tasks && tasks->empty());
    runnable_tasks_.swap(*tasks);
}

void Scheduler::take_all_tasks(TaskQueue* tasks) {
    assert(tasks && tasks->empty());

    runnable_tasks_.swap(*tasks);
    if (suspended_task_count_ > 0) {
        for (auto& item : tickets_) {
            assert(suspended_task_count_ > 0);
            --suspended_task_count_;
            tasks->push(std::move(item.second.task));
        }
    }
}

} // namespace bipolar
