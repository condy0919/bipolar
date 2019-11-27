#include <cstdint>
#include <thread>

#include "bipolar/futures/single_threaded_executor.hpp"

#include <gtest/gtest.h>

using namespace bipolar;
using namespace std::literals;

TEST(SingleThreadedExecutor, running_tasks) {
    SingleThreadedExecutor executor;
    std::uint64_t cnt[3] = {};

    // Schedule a task that runs once and increases a counter
    executor.schedule_task(PendingTask(make_promise([&]() {
        ++cnt[0];
        return AsyncOk(Void{});
    })));

    executor.schedule_task(PendingTask(make_promise([&](Context& ctx) {
        ++cnt[1];
        EXPECT_EQ(ctx.get_executor(), &executor);

        ctx.get_executor()->schedule_task(PendingTask(make_promise([&]() {
            ++cnt[2];
            return AsyncOk(Void{});
        })));
        return AsyncOk(Void{});
    })));

    EXPECT_EQ(cnt[0], 0);
    EXPECT_EQ(cnt[1], 0);
    EXPECT_EQ(cnt[2], 0);

    // We expect that all of the tasks will run to completion including newly
    // scheduled tasks
    executor.run();
    EXPECT_EQ(cnt[0], 1);
    EXPECT_EQ(cnt[1], 1);
    EXPECT_EQ(cnt[2], 1);
}

TEST(SingleThreadedExecutor, suspending_and_resuming_tasks) {
    SingleThreadedExecutor executor;
    std::uint64_t run_cnt[5] = {};
    std::uint64_t resume_cnt[5] = {};

    // Schedule a task that suspends itself and immediately resumes
    executor.schedule_task(
        PendingTask(make_promise([&](Context& ctx) -> AsyncResult<Void, Void> {
            if (++run_cnt[0] == 100) {
                return AsyncOk(Void{});
            }
            ++resume_cnt[0];
            ctx.suspend_task().resume_task();
            return AsyncPending{};
        })));

    // Schedule a task that requires several iterations to complete, each
    // time scheduling another task to resume itself after suspension
    executor.schedule_task(
        PendingTask(make_promise([&](Context& ctx) -> AsyncResult<Void, Void> {
            if (++run_cnt[1] == 100) {
                return AsyncOk(Void{});
            }

            ctx.get_executor()->schedule_task(
                PendingTask(make_promise([&, s = ctx.suspend_task()]() mutable {
                    ++resume_cnt[1];
                    s.resume_task();
                    return AsyncOk(Void{});
                })));
            return AsyncPending{};
        })));

    // Same as the above but use another thread to resume
    executor.schedule_task(
        PendingTask(make_promise([&](Context& ctx) -> AsyncResult<Void, Void> {
            if (++run_cnt[2] == 100) {
                return AsyncOk(Void{});
            }

            std::thread t([&, s = ctx.suspend_task()]() mutable {
                ++resume_cnt[2];
                s.resume_task();
            });
            t.detach();

            return AsyncPending{};
        })));

    // Schedule a task that suspends itself but doesn't actually return pending
    // so it only runs once
    executor.schedule_task(
        PendingTask(make_promise([&](Context& ctx) -> AsyncResult<Void, Void> {
            ++run_cnt[3];
            ctx.suspend_task();
            return AsyncOk(Void{});
        })));

    // Schedule a task that suspends itself and arranges to be resumed on
    // one of two other threads, whichever gets there first
    executor.schedule_task(
        PendingTask(make_promise([&](Context& ctx) -> AsyncResult<Void, Void> {
            if (++run_cnt[4] == 100) {
                return AsyncOk(Void{});
            }

            // Race two threads to resume the task. Either can win
            // This is safe because there threads don't capture references to
            // local variables that might go out of scope when the test exits
            std::thread([s = ctx.suspend_task()]() mutable {
                s.resume_task();
            }).detach();
            std::thread([s = ctx.suspend_task()]() mutable {
                s.resume_task();
            }).detach();
            return AsyncPending{};
        })));

    // We expect the tasks to have been completed after being resumed several
    // times
    executor.run();
    EXPECT_EQ(run_cnt[0], 100);
    EXPECT_EQ(resume_cnt[0], 99);
    EXPECT_EQ(run_cnt[1], 100);
    EXPECT_EQ(resume_cnt[1], 99);
    EXPECT_EQ(run_cnt[2], 100);
    EXPECT_EQ(resume_cnt[2], 99);
    EXPECT_EQ(run_cnt[3], 1);
    EXPECT_EQ(resume_cnt[3], 0);
    EXPECT_EQ(run_cnt[4], 100);
}

TEST(SingleThreadedExecutor, abandoning_tasks) {
    SingleThreadedExecutor executor;
    std::uint64_t run_cnt[4] = {};

    // Schedule a task that returns pending without suspending itself
    // so it is immediately abandoned
    executor.schedule_task(
        PendingTask(make_promise([&]() -> AsyncResult<Void, Void> {
            ++run_cnt[0];
            return AsyncPending{};
        })));

    // Schedule a task that suspends itself but drops the `SuspendedTask`
    // object before returning so it is immediately abandoned
    executor.schedule_task(
        PendingTask(make_promise([&](Context& ctx) -> AsyncResult<Void, Void> {
            ++run_cnt[1];
            ctx.suspend_task(); // ignore result
            return AsyncPending{};
        })));

    // Schedule a task that suspends itself and drops the `SuspendedTask`
    // object from a different thread so it is abandoned concurrently
    executor.schedule_task(
        PendingTask(make_promise([&](Context& ctx) -> AsyncResult<Void, Void> {
            ++run_cnt[2];
            std::thread([s = ctx.suspend_task()]() {}).detach();
            return AsyncPending{};
        })));

    // Schedule a task that creates several suspended task handles and drops
    // them all on the floor
    executor.schedule_task(
        PendingTask(make_promise([&](Context& ctx) -> AsyncResult<Void, Void> {
            ++run_cnt[3];
            SuspendedTask s[3];
            for (std::size_t i = 0; i < 3; ++i) {
                s[i] = ctx.suspend_task();
            }
            return AsyncPending{};
        })));

    // We expect the tasks to have been executed but to have been abandoned
    executor.run();
    EXPECT_EQ(run_cnt[0], 1);
    EXPECT_EQ(run_cnt[1], 1);
    EXPECT_EQ(run_cnt[2], 1);
    EXPECT_EQ(run_cnt[3], 1);
}
