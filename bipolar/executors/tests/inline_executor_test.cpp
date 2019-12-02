#include <cassert>
#include <string>

#include "bipolar/executors/inline_executor.hpp"
#include "bipolar/futures/promise.hpp"

#include <gtest/gtest.h>

using namespace bipolar;

TEST(InlineExecutor, schedule_task) {
    InlineExecutor inline_executor;

    {
        auto p = make_ok_promise<std::string, int>("inline")
                     .and_then(
                         [](const std::string& s) { return AsyncOk(s.size()); })
                     .then([](const AsyncResult<std::size_t, int>& result) {
                         EXPECT_TRUE(result.is_ok());
                         EXPECT_EQ(result.value(), 6);
                         return AsyncOk(Void{});
                     });

        inline_executor.schedule_task(PendingTask(std::move(p)));
    }

    {
        auto p = make_error_promise<std::string, int>(-1)
                     .and_then([](const std::string& s) {
                         EXPECT_TRUE(false);
                         return AsyncOk(Void{});
                     })
                     .or_else([](Context& ctx, const int& err) {
                         EXPECT_THROW(ctx.suspend_task(), std::runtime_error);

                         return AsyncError(Void{});
                     });

        inline_executor.schedule_task(PendingTask(std::move(p)));
    }
}
