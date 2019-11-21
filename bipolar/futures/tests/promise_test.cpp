#include <string>

#include "bipolar/futures/context.hpp"
#include "bipolar/futures/executor.hpp"
#include "bipolar/futures/promise.hpp"

#include <gtest/gtest.h>

using namespace bipolar;
using namespace std::literals;

class DummyContext : public Context {
public:
    struct DummyExecutor : public Executor {
        ~DummyExecutor() override {}

        void schedule_task(PendingTask task) override {
            (void)task;
        }
    };

    Executor& get_executor() const override {
        return const_cast<DummyContext*>(this)->exec_;
    }

    SuspendedTask suspend_task() override {
        __builtin_unreachable();
    }

private:
    DummyExecutor exec_;
};

namespace {
DummyContext ctx;
} // namespace

TEST(Promise, make_result_promise) {
    const auto r0 = make_result_promise<int, std::string>(AsyncOk(42))(ctx);
    EXPECT_TRUE(r0.is_ok());
    EXPECT_EQ(r0.value(), 42);

    const auto r1 =
        make_result_promise<int, std::string>(AsyncError("oops"s))(ctx);
    EXPECT_TRUE(r1.is_error());
    EXPECT_EQ(r1.error(), "oops");

    const auto r2 = make_ok_promise<int, int>(10)(ctx);
    EXPECT_TRUE(r2.is_ok());
    EXPECT_EQ(r2.value(), 10);

    const auto r3 = make_error_promise<int, std::string>("oops"s)(ctx);
    EXPECT_TRUE(r3.is_error());
    EXPECT_EQ(r3.error(), "oops");
}

TEST(Promise, make_promise) {
    const auto r0 = make_promise([](Context& ctx) -> AsyncResult<Void, Void> {
        (void)ctx;
        return AsyncOk(Void{});
    })(ctx);
    EXPECT_TRUE(r0.is_ok());

    const auto r1 = make_promise(
        []() -> AsyncResult<Void, Void> { return AsyncError(Void{}); })(ctx);
    EXPECT_TRUE(r1.is_error());
}

TEST(Promise, then) {
    auto promise = make_ok_promise<int, std::string>(10)
                       .then([](AsyncResult<int, std::string>& result)
                                 -> AsyncResult<int, std::string> {
                           if (result.is_ok()) {
                               return AsyncOk(result.value() * result.value());
                           } else if (result.is_error()) {
                               return AsyncError(result.error() + " ???");
                           }
                           return AsyncPending();
                       })
                       .then([](const AsyncResult<int, std::string>& result)
                                 -> AsyncResult<std::string, std::string> {
                           if (result.is_ok()) {
                               return AsyncOk(std::to_string(result.value()));
                           } else if (result.is_error()) {
                               return AsyncError("error"s);
                           } else {
                               return AsyncPending();
                           }
                       });
    auto result = promise(ctx);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), "100");
}

TEST(Promise, and_then) {
    auto promise =
        make_result_promise<int, std::string>(AsyncOk(10))
            .and_then([](const int& x) -> AsyncResult<int, std::string> {
                if (x % 2 == 0) {
                    return AsyncError("even"s);
                }
                return AsyncOk(x + 1);
            });
    auto result = promise(ctx);
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error(), "even");
}

TEST(Promise, or_else) {
    auto promise =
        make_result_promise<int, std::string>(AsyncError("foo"s))
            .or_else([](const std::string& s) -> AsyncResult<int, int> {
                return AsyncOk(static_cast<int>(s.size()));
            });
    auto result = promise(ctx);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 3);
}

TEST(Promise, inspect) {
    auto promise =
        make_result_promise<int, std::string>(AsyncError("foo"s))
            .inspect([](AsyncResult<int, std::string>& result) {
                if (result.is_ok()) {
                    result.value() = 42;
                } else if (result.is_error()) {
                    result.error() += "bar";
                }
            });
    auto result = promise(ctx);
    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error(), "foobar");
}

TEST(Promise, discard_result) {
    auto promise =
        make_result_promise<int, std::string>(AsyncError("oops"s))
            .discard_result();
    auto result = promise(ctx);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), Void{});
}
