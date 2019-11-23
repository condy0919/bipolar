#include <cstdint>

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

TEST(Future, empty) {
    FakeContext ctx;

    {
        Future<Void, Void> empty;
        EXPECT_EQ(empty.state(), FutureState::EMPTY);
        EXPECT_FALSE(empty);
        EXPECT_TRUE(empty.is_empty());
        EXPECT_FALSE(empty.is_pending());
        EXPECT_FALSE(empty.is_ok());
        EXPECT_FALSE(empty.is_error());
        EXPECT_FALSE(empty.is_ready());
        EXPECT_FALSE(empty(ctx));

        EXPECT_EQ(empty, nullptr);
        EXPECT_EQ(nullptr, empty);
    }

    {
        Future<Void, Void> empty;
        EXPECT_EQ(empty.state(), FutureState::EMPTY);
        EXPECT_FALSE(empty);
        EXPECT_TRUE(empty.is_empty());
        EXPECT_FALSE(empty.is_pending());
        EXPECT_FALSE(empty.is_ok());
        EXPECT_FALSE(empty.is_error());
        EXPECT_FALSE(empty.is_ready());
        EXPECT_FALSE(empty(ctx));
    }

    {
        Future<Void, Void> empty(Promise<Void, Void>(nullptr));
        EXPECT_EQ(empty.state(), FutureState::EMPTY);
        EXPECT_FALSE(empty);
        EXPECT_TRUE(empty.is_empty());
        EXPECT_FALSE(empty.is_pending());
        EXPECT_FALSE(empty.is_ok());
        EXPECT_FALSE(empty.is_error());
        EXPECT_FALSE(empty.is_ready());
        EXPECT_FALSE(empty(ctx));
    }

    {
        Future<Void, Void> empty(AsyncPending{});
        EXPECT_EQ(empty.state(), FutureState::EMPTY);
        EXPECT_FALSE(empty);
        EXPECT_TRUE(empty.is_empty());
        EXPECT_FALSE(empty.is_pending());
        EXPECT_FALSE(empty.is_ok());
        EXPECT_FALSE(empty.is_error());
        EXPECT_FALSE(empty.is_ready());
        EXPECT_FALSE(empty(ctx));
    }
}

TEST(Future, pending_future) {
    FakeContext ctx;
    std::uint64_t cnt = 0;
    Future<int, int> fut(
        make_promise([&](Context& ctx) -> AsyncResult<int, int> {
            (void)ctx;
            if (++cnt == 3) {
                return AsyncOk(42);
            }
            return AsyncPending{};
        }));
    EXPECT_EQ(fut.state(), FutureState::PENDING);
    EXPECT_TRUE(fut);
    EXPECT_FALSE(fut.is_empty());
    EXPECT_TRUE(fut.is_pending());
    EXPECT_FALSE(fut.is_ok());
    EXPECT_FALSE(fut.is_error());
    EXPECT_FALSE(fut.is_ready());

    EXPECT_NE(fut, nullptr);
    EXPECT_NE(nullptr, fut);

    // evaluate the future
    EXPECT_FALSE(fut(ctx));
    EXPECT_EQ(cnt, 1);
    EXPECT_FALSE(fut(ctx));
    EXPECT_EQ(cnt, 2);
    EXPECT_TRUE(fut(ctx));
    EXPECT_EQ(cnt, 3);

    // check the result
    EXPECT_EQ(fut.state(), FutureState::OK);
    EXPECT_TRUE(fut.result().is_ok());
    EXPECT_EQ(fut.result().value(), 42);

    // do something similar but this time produce an error to ensure
    // that this state change works as expected too
    fut = make_promise([&](Context& ctx) -> AsyncResult<int, int> {
        (void)ctx;
        if (++cnt == 5) {
            return AsyncError(42);
        }
        return AsyncPending{};
    });
    EXPECT_EQ(fut.state(), FutureState::PENDING);
    EXPECT_FALSE(fut(ctx));
    EXPECT_EQ(cnt, 4);
    EXPECT_TRUE(fut(ctx));
    EXPECT_EQ(cnt, 5);
    EXPECT_EQ(fut.state(), FutureState::ERROR);
    EXPECT_TRUE(fut.result().is_error());
    EXPECT_EQ(fut.result().error(), 42);
}

TEST(Future, ok_future) {
    FakeContext ctx;
    Future<int, Void> fut(AsyncOk(42));
    EXPECT_EQ(fut.state(), FutureState::OK);
    EXPECT_TRUE(fut);
    EXPECT_FALSE(fut.is_empty());
    EXPECT_FALSE(fut.is_pending());
    EXPECT_TRUE(fut.is_ok());
    EXPECT_FALSE(fut.is_error());
    EXPECT_TRUE(fut.is_ready());
    EXPECT_TRUE(fut(ctx));

    EXPECT_NE(nullptr, fut);
    EXPECT_NE(fut, nullptr);

    // non-destructive access
    EXPECT_TRUE(fut.result().is_ok());
    EXPECT_EQ(fut.result().value(), 42);
    EXPECT_EQ(fut.value(), 42);

    // destructive access
    fut = AsyncOk(43);
    EXPECT_EQ(fut.state(), FutureState::OK);
    EXPECT_EQ(fut.take_result().value(), 43);
    EXPECT_EQ(fut.state(), FutureState::EMPTY);

    fut = AsyncOk(44);
    EXPECT_EQ(fut.state(), FutureState::OK);
    EXPECT_EQ(fut.take_value(), 44);
    EXPECT_EQ(fut.state(), FutureState::EMPTY);
}

TEST(Future, error_future) {
    FakeContext ctx;
    Future<Void, int> fut(AsyncError(42));
    EXPECT_EQ(fut.state(), FutureState::ERROR);
    EXPECT_TRUE(fut);
    EXPECT_FALSE(fut.is_empty());
    EXPECT_FALSE(fut.is_pending());
    EXPECT_FALSE(fut.is_ok());
    EXPECT_TRUE(fut.is_error());
    EXPECT_TRUE(fut.is_ready());
    EXPECT_TRUE(fut(ctx));

    EXPECT_NE(fut, nullptr);
    EXPECT_NE(nullptr, fut);

    // non-destructive access
    EXPECT_TRUE(fut.result().is_error());
    EXPECT_EQ(fut.result().error(), 42);
    EXPECT_EQ(fut.error(), 42);

    // destructive access
    fut = AsyncError(43);
    EXPECT_TRUE(fut.result().is_error());
    EXPECT_EQ(fut.take_result().error(), 43);
    EXPECT_EQ(fut.state(), FutureState::EMPTY);

    fut = AsyncError(44);
    EXPECT_TRUE(fut.result().is_error());
    EXPECT_EQ(fut.take_result().error(), 44);
    EXPECT_EQ(fut.state(), FutureState::EMPTY);
}

TEST(Future, make_future) {
    FakeContext ctx;
    std::uint64_t cnt = 0;
    auto fut = make_future(make_promise([&]() {
        ++cnt;
        return AsyncOk(42);
    }));
    EXPECT_TRUE(fut(ctx));
    EXPECT_EQ(fut.value(), 42);
}
