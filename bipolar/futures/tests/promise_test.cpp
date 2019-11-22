#include <memory>
#include <string>

#include "bipolar/futures/context.hpp"
#include "bipolar/futures/executor.hpp"
#include "bipolar/futures/promise.hpp"

#include <gtest/gtest.h>

using namespace bipolar;
using namespace std::literals;

class DummyContext : public Context {
public:
    Executor* get_executor() const override {
        return nullptr;
    }

    SuspendedTask suspend_task() override {
        __builtin_unreachable();
    }
};

namespace {
DummyContext ctx;
} // namespace

TEST(Promise, empty) {
    {
        Promise<int, int> promise;
        EXPECT_FALSE(promise);
    }

    {
        Promise<int, int> promise(nullptr);
        EXPECT_FALSE(promise);
    }
}

TEST(Promise, invoke) {
    std::uint64_t cnt = 0;

    auto promise = make_promise([&](Context& ctx) -> AsyncResult<Void, Void> {
        (void)ctx;
        if (++cnt == 2) {
            return AsyncOk(Void{});
        }
        return AsyncPending{};
    });
    EXPECT_TRUE(promise);

    auto result = promise(ctx);
    EXPECT_EQ(cnt, 1);
    EXPECT_TRUE(result.is_pending());
    EXPECT_TRUE(promise);

    result = promise(ctx);
    EXPECT_EQ(cnt, 2);
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(promise);
}

TEST(Promise, take_continuation) {
    std::uint64_t cnt = 0;

    auto promise = make_promise([&](Context& ctx) -> AsyncResult<Void, Void> {
                       (void)ctx;
                       ++cnt;
                       return AsyncPending{};
                   }).box();
    EXPECT_TRUE(promise);

    auto f = promise.take_continuation();
    EXPECT_FALSE(promise);
    EXPECT_EQ(cnt, 0);

    auto result = f(ctx);
    EXPECT_EQ(cnt, 1);
    EXPECT_TRUE(result.is_pending());
}

TEST(Promise, assignment_and_swap) {
    Promise<Void, Void> empty;
    EXPECT_FALSE(empty);

    std::uint64_t cnt = 0;

    auto promise = make_promise([&](Context& ctx) -> AsyncResult<Void, Void> {
                       (void)ctx;
                       ++cnt;
                       return AsyncPending{};
                   }).box();
    EXPECT_TRUE(promise);

    auto x(std::move(empty));
    EXPECT_FALSE(x);

    auto y(std::move(promise));
    EXPECT_TRUE(y);
    y(ctx);
    EXPECT_EQ(cnt, 1);

    x.swap(y);
    EXPECT_TRUE(x);
    EXPECT_FALSE(y);
    x(ctx);
    EXPECT_EQ(cnt, 2);

    x = nullptr;
    EXPECT_FALSE(x);

    y = Function<AsyncResult<Void, Void>(Context&)>(
        [&](Context& ctx) -> AsyncResult<Void, Void> {
            (void)ctx;
            cnt *= 2;
            return AsyncPending{};
        });
    EXPECT_TRUE(y);
    y(ctx);
    EXPECT_EQ(cnt, 4);

    x = std::move(y);
    EXPECT_TRUE(x);
    EXPECT_FALSE(y);
    x(ctx);
    EXPECT_EQ(cnt, 8);

    x = std::move(y);
    EXPECT_FALSE(x);
}

TEST(Promise, compare) {
    Promise<Void, Void> promise;
    EXPECT_TRUE(promise == nullptr);
    EXPECT_TRUE(nullptr == promise);
    EXPECT_FALSE(promise != nullptr);
    EXPECT_FALSE(nullptr != promise);

    auto p = make_promise(
        [](Context&) -> AsyncResult<Void, Void> { return AsyncPending{}; });
    EXPECT_FALSE(p == nullptr);
    EXPECT_FALSE(nullptr == p);
    EXPECT_TRUE(p != nullptr);
    EXPECT_TRUE(nullptr != p);
}

TEST(Promise, make_result_promise) {
    const auto r0 = make_result_promise<int, std::string>(AsyncOk(42))(ctx);
    EXPECT_TRUE(r0.is_ok());
    EXPECT_EQ(r0.value(), 42);

    const auto r1 =
        make_result_promise<int, std::string>(AsyncError("oops"s))(ctx);
    EXPECT_TRUE(r1.is_error());
    EXPECT_EQ(r1.error(), "oops");

    const auto r2 = make_result_promise<int, std::string>(AsyncPending{})(ctx);
    EXPECT_TRUE(r2.is_pending());

    const auto r3 = make_ok_promise<int, int>(10)(ctx);
    EXPECT_TRUE(r3.is_ok());
    EXPECT_EQ(r3.value(), 10);

    const auto r4 = make_error_promise<int, std::string>("oops"s)(ctx);
    EXPECT_TRUE(r4.is_error());
    EXPECT_EQ(r4.error(), "oops");
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

    // AsyncResult<int, char>()
    {
        std::uint64_t cnt = 0;
        auto p = make_promise([&]() -> AsyncResult<int, char> {
            ++cnt;
            return AsyncOk(42);
        });
        static_assert(std::is_same_v<int, decltype(p)::value_type>);
        static_assert(std::is_same_v<char, decltype(p)::error_type>);

        auto result = p(ctx);
        EXPECT_EQ(cnt, 1);
        EXPECT_TRUE(result.is_ok());
        EXPECT_EQ(result.value(), 42);
        EXPECT_FALSE(p);
    }

    // AsyncResult<int, Void>()
    {
        std::uint64_t cnt = 0;
        auto p = make_promise([&]() {
            ++cnt;
            return AsyncOk<int>(42);
        });
        static_assert(std::is_same_v<int, decltype(p)::value_type>);
        static_assert(std::is_same_v<Void, decltype(p)::error_type>);

        auto result = p(ctx);
        EXPECT_EQ(cnt, 1);
        EXPECT_TRUE(result.is_ok());
        EXPECT_EQ(result.value(), 42);
        EXPECT_FALSE(p);
    }

    // AsyncResult<Void, int>
    {
        std::uint64_t cnt = 0;
        auto p = make_promise([&]() {
            ++cnt;
            return AsyncError<int>(42);
        });
        static_assert(std::is_same_v<Void, decltype(p)::value_type>);
        static_assert(std::is_same_v<int, decltype(p)::error_type>);

        auto result = p(ctx);
        EXPECT_EQ(cnt, 1);
        EXPECT_TRUE(result.is_error());
        EXPECT_EQ(result.error(), 42);
        EXPECT_FALSE(p);
    }

    // AsyncResult<Void, Void>
    {
        std::uint64_t cnt = 0;
        auto p = make_promise([&]() {
            ++cnt;
            return AsyncPending{};
        });
        static_assert(std::is_same_v<Void, decltype(p)::value_type>);
        static_assert(std::is_same_v<Void, decltype(p)::error_type>);

        auto result = p(ctx);
        EXPECT_EQ(cnt, 1);
        EXPECT_TRUE(result.is_pending());
        EXPECT_TRUE(p);
    }

    // PromiseImpl
    {
        std::uint64_t cnt1 = 0, cnt2 = 0;
        auto p = make_promise([&cnt1, &cnt2]() {
            ++cnt1;
            return make_promise([&cnt2]() -> AsyncResult<int, char> {
                if (++cnt2 == 2) {
                    return AsyncOk(42);
                }
                return AsyncPending{};
            });
        });
        static_assert(std::is_same_v<int, decltype(p)::value_type>);
        static_assert(std::is_same_v<char, decltype(p)::error_type>);

        auto result = p(ctx);
        EXPECT_EQ(cnt1, 1);
        EXPECT_EQ(cnt2, 1);
        EXPECT_TRUE(result.is_pending());
        EXPECT_TRUE(p);

        result = p(ctx);
        EXPECT_EQ(cnt1, 1);
        EXPECT_EQ(cnt2, 2);
        EXPECT_TRUE(result.is_ok());
        EXPECT_EQ(result.value(), 42);
        EXPECT_FALSE(p);
    }
}

TEST(Promise, then) {
    // Chaining on OK
    {
        std::uint64_t cnt = 0;
        auto p = make_ok_promise<int, int>(42).then(
            [&](const AsyncResult<int, int>& result)
                -> AsyncResult<Void, Void> {
                (void)result;
                if (++cnt == 2) {
                    return AsyncOk(Void{});
                }
                return AsyncPending{};
            });

        auto result = p(ctx);
        EXPECT_TRUE(p);
        EXPECT_EQ(cnt, 1);
        EXPECT_TRUE(result.is_pending());

        result = p(ctx);
        EXPECT_FALSE(p);
        EXPECT_EQ(cnt, 2);
        EXPECT_TRUE(result.is_ok());
    }

    // Chaining on ERROR
    {
        std::uint64_t cnt = 0;
        auto p = make_error_promise<int, int>(42).then(
            [&](const AsyncResult<int, int>& result)
                -> AsyncResult<Void, Void> {
                (void)result;
                if (++cnt == 2) {
                    return AsyncOk(Void{});
                }
                return AsyncPending{};
            });

        auto result = p(ctx);
        EXPECT_TRUE(p);
        EXPECT_EQ(cnt, 1);
        EXPECT_TRUE(result.is_pending());

        result = p(ctx);
        EXPECT_FALSE(p);
        EXPECT_EQ(cnt, 2);
        EXPECT_TRUE(result.is_ok());
    }

    // All argument signatures
    {
        std::uint64_t cnt = 0;
        auto p =
            make_ok_promise<int, int>(42)
                .then([&](AsyncResult<int, int>& result)
                          -> AsyncResult<int, int> {
                    ++cnt;
                    return AsyncOk(result.value() + 1);
                })
                .then([&](const AsyncResult<int, int>& result)
                          -> AsyncResult<int, int> {
                    ++cnt;
                    return AsyncOk(result.value() + 1);
                })
                .then([&](Context& ctx, AsyncResult<int, int>& result)
                          -> AsyncResult<int, int> {
                    (void)ctx;
                    ++cnt;
                    return AsyncOk(result.value() + 1);
                })
                .then([&](Context& ctx, const AsyncResult<int, int>& result)
                          -> AsyncResult<int, int> {
                    (void)ctx;
                    ++cnt;
                    return AsyncOk(result.value() + 1);
                });

        auto result = p(ctx);
        EXPECT_FALSE(p);
        EXPECT_EQ(cnt, 4);
        EXPECT_TRUE(result.is_ok());
        EXPECT_EQ(result.value(), 46);
    }
}

TEST(Promise, and_then) {
    // Chaining on OK
    {
        std::uint64_t cnt = 0;
        auto p = make_ok_promise<int, int>(42).and_then(
            [&](const int& x) -> AsyncResult<Void, int> {
                (void)x;
                if (++cnt == 2) {
                    return AsyncError(-1);
                }
                return AsyncPending{};
            });

        auto result = p(ctx);
        EXPECT_TRUE(p);
        EXPECT_EQ(cnt, 1);
        EXPECT_TRUE(result.is_pending());

        result = p(ctx);
        EXPECT_FALSE(p);
        EXPECT_EQ(cnt, 2);
        EXPECT_TRUE(result.is_error());
        EXPECT_EQ(result.error(), -1);
    }

    // Chainging on ERROR
    {
        std::uint64_t cnt = 0;
        auto p = make_error_promise<int, int>(42).and_then(
            [&](const int& x) -> AsyncResult<Void, int> {
                (void)x;
                ++cnt;
                return AsyncPending{};
            });

        auto result = p(ctx);
        EXPECT_FALSE(p);
        EXPECT_EQ(cnt, 0);
        EXPECT_TRUE(result.is_error());
        EXPECT_EQ(result.error(), 42);
    }

    // All argument signatures
    {
        std::uint64_t cnt = 0;
        auto p =
            make_ok_promise<int, int>(42)
                .and_then([&](int& x) -> AsyncResult<int, int> {
                    ++cnt;
                    return AsyncOk(x + 1);
                })
                .and_then([&](const int& x) -> AsyncResult<int, int> {
                    ++cnt;
                    return AsyncOk(x + 1);
                })
                .and_then([&](Context& ctx, int& x) -> AsyncResult<int, int> {
                    (void)ctx;
                    ++cnt;
                    return AsyncOk(x + 1);
                })
                .and_then(
                    [&](Context& ctx, const int& x) -> AsyncResult<int, int> {
                        (void)ctx;
                        ++cnt;
                        return AsyncOk(x + 1);
                    });

        auto result = p(ctx);
        EXPECT_FALSE(p);
        EXPECT_EQ(cnt, 4);
        EXPECT_TRUE(result.is_ok());
        EXPECT_EQ(result.value(), 46);
    }
}

TEST(Promise, or_else) {
    // Chaining on OK
    {
        std::uint64_t cnt = 0;
        auto p = make_ok_promise<int, int>(42).or_else(
            [&](const int& x) -> AsyncResult<int, int> {
                (void)x;
                ++cnt;
                return AsyncPending{};
            });

        auto result = p(ctx);
        EXPECT_FALSE(p);
        EXPECT_EQ(cnt, 0);
        EXPECT_TRUE(result.is_ok());
        EXPECT_EQ(result.value(), 42);
    }

    // Chaining on ERROR
    {
        std::uint64_t cnt = 0;
        auto p = make_error_promise<int, int>(42).or_else(
            [&](const int& x) -> AsyncResult<int, int> {
                (void)x;
                ++cnt;
                return AsyncPending{};
            });

        auto result = p(ctx);
        EXPECT_TRUE(p);
        EXPECT_EQ(cnt, 1);
        EXPECT_TRUE(result.is_pending());
    }

    // All argument signatures
    {
        std::uint64_t cnt = 0;
        auto p =
            make_error_promise<int, int>(42)
                .or_else([&](int& err) -> AsyncResult<int, int> {
                    ++cnt;
                    return AsyncError(err + 1);
                })
                .or_else([&](const int& err) -> AsyncResult<int, int> {
                    ++cnt;
                    return AsyncError(err + 1);
                })
                .or_else([&](Context& ctx, int& err) -> AsyncResult<int, int> {
                    (void)ctx;
                    ++cnt;
                    return AsyncError(err + 1);
                })
                .or_else(
                    [&](Context& ctx, const int& err) -> AsyncResult<int, int> {
                        (void)ctx;
                        ++cnt;
                        return AsyncError(err + 1);
                    });

        auto result = p(ctx);
        EXPECT_EQ(cnt, 4);
        EXPECT_TRUE(result.is_error());
        EXPECT_EQ(result.error(), 46);
        EXPECT_FALSE(p);
    }
}

TEST(Promise, inspect) {
    // Inspects without `Context&`
    {
        auto promise = make_result_promise<int, std::string>(AsyncError("foo"s))
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

    // Inspects with `Context&`
    {
        auto promise =
            make_result_promise<int, std::string>(AsyncOk(42))
                .inspect([](Context& ctx,
                            const AsyncResult<int, std::string>& result) {
                    (void)ctx;
                    EXPECT_TRUE(result.is_ok());
                    EXPECT_EQ(result.value(), 42);
                });

        auto result = promise(ctx);
        EXPECT_TRUE(result.is_ok());
        EXPECT_FALSE(promise);
    }
}

TEST(Promise, discard_result) {
    auto promise = make_result_promise<int, std::string>(AsyncError("oops"s))
                       .discard_result();
    auto result = promise(ctx);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), Void{});
}

TEST(Promise, join_promises) {
    std::uint64_t cnt = 0;

    auto p = join_promises(
        make_ok_promise<int, int>(42),
        make_error_promise<char, char>('a').or_else([](const char& err) {
            (void)err;
            return AsyncError('y');
        }),
        make_promise([&]() -> AsyncResult<std::string, int> {
            if (++cnt == 2) {
                return AsyncOk("oops"s);
            }
            return AsyncPending{};
        }));
    EXPECT_TRUE(p);

    auto result = p(ctx);
    EXPECT_TRUE(p);
    EXPECT_EQ(cnt, 1);
    EXPECT_TRUE(result.is_pending());

    result = p(ctx);
    EXPECT_FALSE(p);
    EXPECT_EQ(cnt, 2);
    EXPECT_TRUE(result.is_ok());

    auto& [r0, r1, r2] = result.value();
    EXPECT_EQ(r0.value(), 42);
    EXPECT_EQ(r1.error(), 'y');
    EXPECT_EQ(r2.value(), "oops");
}

TEST(Promise, join_promises_with_move_only_result) {
    auto p =
        join_promises(make_ok_promise<std::unique_ptr<int>, int>(
                          std::make_unique<int>(10)),
                      make_error_promise<int, std::unique_ptr<int>>(
                          std::make_unique<int>(11)))
            .then(
                [](AsyncResult<
                    std::tuple<AsyncResult<std::unique_ptr<int>, int>,
                               AsyncResult<int, std::unique_ptr<int>>>,
                    Void>& wrapper) -> AsyncResult<std::unique_ptr<int>, int> {
                    auto [r0, r1] = wrapper.take_value();
                    if (r0.is_ok() && r1.is_error()) {
                        int value = *r0.take_value() + *r1.take_error();
                        return AsyncOk(std::make_unique<int>(value));
                    }
                    return AsyncError(-1);
                });
    EXPECT_TRUE(p);

    auto result = p(ctx);
    EXPECT_FALSE(p);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(*result.value(), 21);
}

TEST(Promise, join_vector_promise) {
    std::vector<Promise<int, int>> promises;
    promises.push_back(make_ok_promise<int, int>(42));
    promises.push_back(make_error_promise<int, int>(-1));

    auto p = join_promise_vector(std::move(promises));
    EXPECT_TRUE(p);

    auto result = p(ctx);
    EXPECT_FALSE(p);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value()[0].value(), 42);
    EXPECT_EQ(result.value()[1].error(), -1);
}
