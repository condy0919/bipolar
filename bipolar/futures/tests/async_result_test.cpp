#include <string>

#include "bipolar/core/movable.hpp"
#include "bipolar/core/void.hpp"
#include "bipolar/futures/async_result.hpp"

#include <gtest/gtest.h>

using namespace bipolar;
using namespace std::literals;

struct MoveOnly : Movable {};

TEST(AsyncResult, Basic) {
    AsyncResult<Void, Void> good = AsyncOk(Void{});
    EXPECT_TRUE(good.is_ok());
    EXPECT_FALSE(good.is_error());
    EXPECT_FALSE(good.is_pending());

    AsyncResult<Void, Void> bad = AsyncError(Void{});
    EXPECT_TRUE(bad.is_error());
    EXPECT_FALSE(bad.is_ok());
    EXPECT_FALSE(bad.is_pending());

    AsyncResult<Void, Void> pending = AsyncPending();
    EXPECT_TRUE(pending.is_pending());
    EXPECT_FALSE(pending.is_ok());
    EXPECT_FALSE(pending.is_error());

    AsyncResult<Void, Void> default_init;
    EXPECT_TRUE(default_init.is_pending());
    EXPECT_FALSE(default_init.is_ok());
    EXPECT_FALSE(default_init.is_error());
}

TEST(AsyncResult, Move) {
    AsyncResult<int, int> good = AsyncOk(42);
    EXPECT_TRUE(good.is_ok());

    auto tmpcopy = good;
    EXPECT_TRUE(good.is_ok());
    EXPECT_TRUE(tmpcopy.is_ok());
    EXPECT_EQ(tmpcopy.value(), 42);

    auto tmpmove = std::move(good);
    EXPECT_TRUE(tmpmove.is_ok());
    EXPECT_EQ(tmpmove.value(), 42);
    EXPECT_FALSE(good.is_ok());
    EXPECT_TRUE(good.is_pending());
}

TEST(AsyncResult, MoveOnly) {
    AsyncResult<MoveOnly, Void> good = AsyncOk(MoveOnly{});
    EXPECT_TRUE(good.is_ok());

    // The story behind the test
    // https://github.com/condy0919/bipolar/issues/21
    AsyncResult<MoveOnly, Void> tmpmove = std::move(good);
    EXPECT_TRUE(tmpmove.is_ok());
    EXPECT_FALSE(good.is_ok());
    EXPECT_TRUE(good.is_pending());
}

TEST(AsyncResult, Take) {
    AsyncResult<int, std::string> good = AsyncOk(42);
    EXPECT_EQ(good.take_value(), 42);
    EXPECT_TRUE(good.is_pending());

    good = AsyncError("foo"s);
    EXPECT_TRUE(good.is_error());
    EXPECT_EQ(good.take_error(), "foo");
    EXPECT_TRUE(good.is_pending());
}

TEST(AsyncResult, Swap) {
    AsyncResult<int, std::string> good = AsyncOk(13);
    AsyncResult<int, std::string> bad = AsyncError("foo"s);

    swap(good, bad);
    EXPECT_TRUE(good.is_error());
    EXPECT_EQ(good.error(), "foo");
    EXPECT_TRUE(bad.is_ok());
    EXPECT_EQ(bad.value(), 13);
}

TEST(AsyncResult, constexpr) {
    constexpr AsyncResult<int, int> good = AsyncOk(1);
    static_assert(good.is_ok());
    static_assert(good.value() == 1);

    constexpr AsyncResult<int, int> bad = AsyncError(-4);
    static_assert(bad.is_error());
    static_assert(bad.error() == -4);

    constexpr AsyncResult<int, int> pending = AsyncPending();
    static_assert(pending.is_pending());
}
