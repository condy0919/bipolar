//! InlineExecutor
//!
//! See `InlineExecutor` for details
//!

#ifndef BIPOLAR_EXECUTORS_INLINE_EXECUTOR_HPP_
#define BIPOLAR_EXECUTORS_INLINE_EXECUTOR_HPP_

#include "bipolar/futures/executor.hpp"

#include <cassert>
#include <stdexcept>

#include <boost/noncopyable.hpp>

namespace bipolar {
/// InlineExecutor
///
/// An executor which always creates execution "inline".
/// Therefore, the execution it creates always blocks the execution of its
/// client.
///
/// # Examples
///
/// ```
/// InlineExecutor inline_executor;
///
/// auto p = make_ok_promise<std::string, int>("inline")
///     .and_then([](const std::string& s) {
///         return Ok(s.size());
///     })
///     .then([](const Result<std::size_t, int>& result) {
///         assert(result.is_ok());
///         assert(result.value() == 6);
///         return Ok(Void{});
///     });
///
/// inline_executor.schedule_task(PendingTask(std::move(p)));
/// ```
class InlineExecutor final : public Executor, public boost::noncopyable {
public:
    InlineExecutor() noexcept : ctx_(this) {}

    ~InlineExecutor() override = default;

    void schedule_task(PendingTask task) override {
        const bool finished = task(ctx_);
        assert(finished == !task);
        (void)finished;
    }

private:
    class ContextImpl : public Context {
    public:
        explicit ContextImpl(InlineExecutor* executor) : executor_(executor) {}

        ~ContextImpl() override = default;

        InlineExecutor* get_executor() const override {
            return executor_;
        }

        SuspendedTask suspend_task() override {
            throw std::runtime_error("InlineExecutor doesn't support suspend");
        }

    private:
        InlineExecutor* const executor_;
    };

    ContextImpl ctx_;
};
} // namespace bipolar

#endif
