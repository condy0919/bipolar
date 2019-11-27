#include <cstdint>

#include "bipolar/futures/scheduler.hpp"

#include <gtest/gtest.h>

using namespace bipolar;
using namespace std::literals;

class FakeContext : public Context {
public:
    Executor* get_executor() const override {
        return nullptr;
    }

    SuspendedTask suspend_task() override {
        return {};
    }
};

static PendingTask make_pending_task(std::uint64_t* cnt) {
    return PendingTask(make_promise([cnt]() -> AsyncResult<Void, Void> {
        (*cnt)++;
        return AsyncOk(Void{});
    }));
}

TEST(Scheduler, initial_state) {
    Scheduler scheduler;
    EXPECT_FALSE(scheduler.has_runnable_tasks());
    EXPECT_FALSE(scheduler.has_suspended_tasks());
    EXPECT_FALSE(scheduler.has_outstanding_tickets());
}

TEST(Scheduler, schedule_task) {
    Scheduler scheduler;
    Scheduler::TaskQueue tasks;
    FakeContext ctx;
    std::uint64_t cnt[3] = {};

    // Initially there are no tasks
    scheduler.take_runnable_tasks(&tasks);
    EXPECT_TRUE(tasks.empty());

    // Schedule and run one task
    scheduler.schedule_task(make_pending_task(&cnt[0]));
    EXPECT_TRUE(scheduler.has_runnable_tasks());
    EXPECT_FALSE(scheduler.has_suspended_tasks());
    EXPECT_FALSE(scheduler.has_outstanding_tickets());
    scheduler.take_all_tasks(&tasks);
    EXPECT_EQ(tasks.size(), 1);
    EXPECT_TRUE(tasks.front()(ctx));
    EXPECT_EQ(cnt[0], 1);
    EXPECT_FALSE(tasks.front());
    tasks.pop();
    EXPECT_TRUE(tasks.empty());

    // Run a couple more, ensure that they come out in queue order
    scheduler.schedule_task(make_pending_task(&cnt[0]));
    scheduler.schedule_task(make_pending_task(&cnt[1]));
    scheduler.schedule_task(make_pending_task(&cnt[2]));
    EXPECT_TRUE(scheduler.has_runnable_tasks());
    EXPECT_FALSE(scheduler.has_suspended_tasks());
    EXPECT_FALSE(scheduler.has_outstanding_tickets());
    scheduler.take_all_tasks(&tasks);
    EXPECT_EQ(tasks.size(), 3);
    EXPECT_TRUE(tasks.front()(ctx));
    EXPECT_EQ(cnt[0], 2);
    EXPECT_EQ(cnt[1], 0);
    EXPECT_EQ(cnt[2], 0);
    tasks.pop();
    EXPECT_TRUE(tasks.front()(ctx));
    EXPECT_EQ(cnt[0], 2);
    EXPECT_EQ(cnt[1], 1);
    EXPECT_EQ(cnt[2], 0);
    tasks.pop();
    EXPECT_TRUE(tasks.front()(ctx));
    EXPECT_EQ(cnt[0], 2);
    EXPECT_EQ(cnt[1], 1);
    EXPECT_EQ(cnt[2], 1);
    tasks.pop();
    EXPECT_TRUE(tasks.empty());

    // Once we are done, no tasks are left
    scheduler.take_all_tasks(&tasks);
    EXPECT_TRUE(tasks.empty());
}

TEST(Scheduler, ticket_obtain_finalize_without_task) {
    Scheduler scheduler;

    SuspendedTask::Ticket t = scheduler.obtain_ticket();
    EXPECT_FALSE(scheduler.has_runnable_tasks());
    EXPECT_FALSE(scheduler.has_suspended_tasks());
    EXPECT_TRUE(scheduler.has_outstanding_tickets());

    PendingTask task;
    scheduler.finalize_ticket(t, &task);
    EXPECT_FALSE(scheduler.has_runnable_tasks());
    EXPECT_FALSE(scheduler.has_suspended_tasks());
    EXPECT_FALSE(scheduler.has_outstanding_tickets());
}

TEST(Scheduler, ticket_obtain_finalize_with_task) {
    Scheduler scheduler;

    SuspendedTask::Ticket t = scheduler.obtain_ticket();
    EXPECT_FALSE(scheduler.has_runnable_tasks());
    EXPECT_FALSE(scheduler.has_suspended_tasks());
    EXPECT_TRUE(scheduler.has_outstanding_tickets());

    std::uint64_t cnt = 0;
    PendingTask p = make_pending_task(&cnt);
    scheduler.finalize_ticket(t, &p);
    EXPECT_FALSE(scheduler.has_runnable_tasks());
    EXPECT_FALSE(scheduler.has_suspended_tasks());
    EXPECT_FALSE(scheduler.has_outstanding_tickets());
}

TEST(Scheduler, ticket_obtain_finalize_release) {
    Scheduler scheduler;

    SuspendedTask::Ticket t = scheduler.obtain_ticket(/*initial_refs = */ 2);
    scheduler.duplicate_ticket(t);
    EXPECT_FALSE(scheduler.has_runnable_tasks());
    EXPECT_FALSE(scheduler.has_suspended_tasks());
    EXPECT_TRUE(scheduler.has_outstanding_tickets());

    std::uint64_t cnt = 0;
    PendingTask p = make_pending_task(&cnt);
    scheduler.finalize_ticket(t, &p);
    EXPECT_FALSE(scheduler.has_runnable_tasks());
    EXPECT_TRUE(scheduler.has_suspended_tasks());
    EXPECT_TRUE(scheduler.has_outstanding_tickets());
    EXPECT_FALSE(p); // took ownership

    p = scheduler.release_ticket(t);
    EXPECT_FALSE(scheduler.has_runnable_tasks());
    EXPECT_TRUE(scheduler.has_suspended_tasks());
    EXPECT_TRUE(scheduler.has_outstanding_tickets());
    EXPECT_FALSE(p);

    p = scheduler.release_ticket(t);
    EXPECT_FALSE(scheduler.has_runnable_tasks());
    EXPECT_FALSE(scheduler.has_suspended_tasks());
    EXPECT_FALSE(scheduler.has_outstanding_tickets());
}

TEST(Scheduler, ticket_obtain_duplicate_finalize_resume) {
    Scheduler scheduler;

    SuspendedTask::Ticket t = scheduler.obtain_ticket(/*initial_refs = */ 2);
    scheduler.duplicate_ticket(t);
    EXPECT_FALSE(scheduler.has_runnable_tasks());
    EXPECT_FALSE(scheduler.has_suspended_tasks());
    EXPECT_TRUE(scheduler.has_outstanding_tickets());

    std::uint64_t cnt = 0;
    PendingTask p = make_pending_task(&cnt);
    scheduler.finalize_ticket(t, &p);
    EXPECT_FALSE(scheduler.has_runnable_tasks());
    EXPECT_TRUE(scheduler.has_suspended_tasks());
    EXPECT_TRUE(scheduler.has_outstanding_tickets());
    EXPECT_FALSE(p); // took ownership

    scheduler.resume_task_with_ticket(t);
    EXPECT_TRUE(scheduler.has_runnable_tasks());
    EXPECT_FALSE(scheduler.has_suspended_tasks());
    EXPECT_TRUE(scheduler.has_outstanding_tickets());

    p = scheduler.release_ticket(t);
    EXPECT_TRUE(scheduler.has_runnable_tasks());
    EXPECT_FALSE(scheduler.has_suspended_tasks());
    EXPECT_FALSE(scheduler.has_outstanding_tickets());
    EXPECT_FALSE(p);

    Scheduler::TaskQueue tasks;
    scheduler.take_runnable_tasks(&tasks);
    EXPECT_EQ(tasks.size(), 1);

    FakeContext ctx;
    tasks.front()(ctx);
    EXPECT_EQ(cnt, 1);
}

TEST(Scheduler, ticket_obtain_release_finalize) {
    Scheduler scheduler;

    SuspendedTask::Ticket t = scheduler.obtain_ticket(/*initial_refs = */ 2);
    EXPECT_FALSE(scheduler.has_runnable_tasks());
    EXPECT_FALSE(scheduler.has_suspended_tasks());
    EXPECT_TRUE(scheduler.has_outstanding_tickets());

    PendingTask p = scheduler.release_ticket(t);
    EXPECT_FALSE(scheduler.has_runnable_tasks());
    EXPECT_FALSE(scheduler.has_suspended_tasks());
    EXPECT_TRUE(scheduler.has_outstanding_tickets());
    EXPECT_FALSE(p);

    std::uint64_t cnt = 0;
    p = make_pending_task(&cnt);
    scheduler.finalize_ticket(t, &p);
    EXPECT_FALSE(scheduler.has_runnable_tasks());
    EXPECT_FALSE(scheduler.has_suspended_tasks());
    EXPECT_FALSE(scheduler.has_outstanding_tickets());
    EXPECT_TRUE(p);
}

TEST(Scheduler, ticket_obtain_resume_finalize) {
    Scheduler scheduler;

    SuspendedTask::Ticket t = scheduler.obtain_ticket(/*initial_refs = */ 2);
    EXPECT_FALSE(scheduler.has_runnable_tasks());
    EXPECT_FALSE(scheduler.has_suspended_tasks());
    EXPECT_TRUE(scheduler.has_outstanding_tickets());

    scheduler.resume_task_with_ticket(t);
    EXPECT_FALSE(scheduler.has_runnable_tasks());
    EXPECT_FALSE(scheduler.has_suspended_tasks());
    EXPECT_TRUE(scheduler.has_outstanding_tickets());

    std::uint64_t cnt = 0;
    PendingTask p = make_pending_task(&cnt);
    scheduler.finalize_ticket(t, &p);
    EXPECT_TRUE(scheduler.has_runnable_tasks());
    EXPECT_FALSE(scheduler.has_suspended_tasks());
    EXPECT_FALSE(scheduler.has_outstanding_tickets());
    EXPECT_FALSE(p);

    Scheduler::TaskQueue tasks;
    scheduler.take_all_tasks(&tasks);
    EXPECT_EQ(tasks.size(), 1);

    FakeContext ctx;
    tasks.front()(ctx);
    EXPECT_EQ(cnt, 1);
}

TEST(Scheduler, take_all_tasks) {
    Scheduler scheduler;
    Scheduler::TaskQueue tasks;
    FakeContext ctx;
    std::uint64_t cnt[6] = {};

    // Initially there are no tasks
    scheduler.take_all_tasks(&tasks);
    EXPECT_TRUE(tasks.empty());

    // Schedule a task
    scheduler.schedule_task(make_pending_task(&cnt[0]));
    EXPECT_TRUE(scheduler.has_runnable_tasks());

    // Suspend a task and finalize it without resumption
    // This does not leave an outstanding ticket
    SuspendedTask::Ticket t1 = scheduler.obtain_ticket();
    PendingTask p1 = make_pending_task(&cnt[1]);
    scheduler.finalize_ticket(t1, &p1);
    EXPECT_TRUE(p1); // took ownership

    // Suspend a task and duplicate its ticket
    // This leaves an outstanding ticket with an associated task
    SuspendedTask::Ticket t2 = scheduler.obtain_ticket();
    PendingTask p2 = make_pending_task(&cnt[2]);
    scheduler.duplicate_ticket(t2);
    scheduler.finalize_ticket(t2, &p2);
    EXPECT_FALSE(p2); // didn't take ownership

    // Suspend a task, duplicate its ticket, then release it
    // This does not leave an outstanding ticket
    SuspendedTask::Ticket t3 = scheduler.obtain_ticket();
    PendingTask p3 = make_pending_task(&cnt[3]);
    scheduler.duplicate_ticket(t3);
    scheduler.finalize_ticket(t3, &p3);
    EXPECT_FALSE(p3); // didn't take ownership
    p3 = scheduler.release_ticket(t3);
    EXPECT_TRUE(p3);

    // Suspend a task, duplicate its ticket, the resume it
    // This adds a runnable task but does not leave an outstanding ticket
    SuspendedTask::Ticket t4 = scheduler.obtain_ticket();
    PendingTask p4 = make_pending_task(&cnt[4]);
    scheduler.duplicate_ticket(t4);
    scheduler.finalize_ticket(t4, &p4);
    EXPECT_FALSE(p4); // didn't take ownership
    EXPECT_TRUE(scheduler.resume_task_with_ticket(t4));

    // Suspend a task, duplicate its ticket twice, the resume it
    // This adds a runnable task and leave an outstanding ticket without an
    // associated task
    SuspendedTask::Ticket t5 = scheduler.obtain_ticket();
    PendingTask p5 = make_pending_task(&cnt[5]);
    scheduler.duplicate_ticket(t5);
    scheduler.duplicate_ticket(t5);
    scheduler.finalize_ticket(t5, &p5);
    EXPECT_FALSE(p5);
    EXPECT_TRUE(scheduler.resume_task_with_ticket(t5));

    // Now take all tasks
    // we expect to find tasks that were runnable or associated with
    // outstanding tickets. Those outstanding tickets will remain, however
    // they no longer have an associated task (cannot subsequently be resumed)
    EXPECT_TRUE(scheduler.has_runnable_tasks());
    EXPECT_TRUE(scheduler.has_suspended_tasks());
    EXPECT_TRUE(scheduler.has_outstanding_tickets());
    // FIXME
    // scheduler.take_all_tasks(&tasks);
    // EXPECT_FALSE(scheduler.has_runnable_tasks());
    // EXPECT_FALSE(scheduler.has_suspended_tasks());
    // EXPECT_TRUE(scheduler.has_outstanding_tickets());

    // // Check that we obtained the tasks we expected to obtain, by running them
    // EXPECT_EQ(tasks.size(), 4);
    // while (!tasks.empty()) {
    //     auto task = std::move(tasks.front());
    //     task(ctx);
    //     tasks.pop();
    // }
    // EXPECT_EQ(cnt[0], 1);
    // EXPECT_EQ(cnt[1], 0);
    // EXPECT_EQ(cnt[2], 1);
    // EXPECT_EQ(cnt[3], 0);
    // EXPECT_EQ(cnt[4], 1);
    // EXPECT_EQ(cnt[5], 1);
    //
    // // Now that everything is gone, taking all tasks should return an empty set
    // scheduler.take_all_tasks(&tasks);
    // EXPECT_FALSE(scheduler.has_runnable_tasks());
    // EXPECT_FALSE(scheduler.has_suspended_tasks());
    // EXPECT_TRUE(scheduler.has_outstanding_tickets());
    // EXPECT_TRUE(tasks.empty());
}
