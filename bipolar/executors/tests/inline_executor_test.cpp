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
                         [](const std::string& s) { return Ok(s.size()); })
                     .then([](const Result<std::size_t, int>& result) {
                         EXPECT_TRUE(result.is_ok());
                         EXPECT_EQ(result.value(), 6);
                         return Ok(Void{});
                     });

        inline_executor.schedule_task(PendingTask(std::move(p)));
    }

    {
        auto p = make_error_promise<std::string, int>(-1)
                     .and_then([](const std::string& s) {
                         EXPECT_TRUE(false);
                         return Ok(Void{});
                     })
                     .or_else([](Context& ctx, const int& err) {
                         EXPECT_THROW(ctx.suspend_task(), std::runtime_error);

                         return Err(Void{});
                     });

        inline_executor.schedule_task(PendingTask(std::move(p)));
    }
}
