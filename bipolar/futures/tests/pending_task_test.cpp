#include <cstdint>

#include "bipolar/futures/pending_task.hpp"
#include "bipolar/futures/promise.hpp"

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

TEST(PendingTask, empty_task) {
    {
        PendingTask empty;
        EXPECT_FALSE(empty);
        EXPECT_FALSE(empty.take_promise());
    }

    {
        PendingTask empty(Promise<Void, Void>(nullptr));
        EXPECT_FALSE(empty);
        EXPECT_FALSE(empty.take_promise());
    }

    {
        PendingTask empty(Promise<int, int>(nullptr));
        EXPECT_FALSE(empty);
        EXPECT_FALSE(empty.take_promise());
    }
}

TEST(PendingTask, non_empty_task) {
    FakeContext ctx;

    {
        std::uint64_t cnt = 0;
        PendingTask task(make_promise([&]() -> AsyncResult<Void, Void> {
            if (++cnt == 3) {
                return AsyncOk(Void{});
            }
            return AsyncPending{};
        }));
        EXPECT_TRUE(task);

        EXPECT_FALSE(task(ctx));
        EXPECT_EQ(cnt, 1);
        EXPECT_TRUE(task);

        EXPECT_FALSE(task(ctx));
        EXPECT_EQ(cnt, 2);
        EXPECT_TRUE(task);

        EXPECT_TRUE(task(ctx));
        EXPECT_EQ(cnt, 3);
        EXPECT_FALSE(task);
        EXPECT_FALSE(task.take_promise());
    }

    {
        std::uint64_t cnt = 0;
        PendingTask task(make_promise([&]() -> AsyncResult<int, Void> {
            if (++cnt == 2) {
                return AsyncOk(0);
            }
            return AsyncPending{};
        }));
        EXPECT_TRUE(task);

        PendingTask task_move(std::move(task));
        EXPECT_TRUE(task_move);
        EXPECT_FALSE(task);

        PendingTask task_movemove;
        task_movemove = std::move(task_move);
        EXPECT_TRUE(task_movemove);
        EXPECT_FALSE(task_move);

        Promise<Void, Void> promise = task_movemove.take_promise();
        EXPECT_TRUE(promise);
        EXPECT_TRUE(promise(ctx).is_pending());
        EXPECT_EQ(cnt, 1);

        EXPECT_TRUE(promise(ctx).is_ok());
        EXPECT_EQ(cnt, 2);
        EXPECT_FALSE(promise);
    }
}
