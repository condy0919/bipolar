#include "bipolar/core/function.hpp"

#include <array>
#include <vector>

#include <gtest/gtest.h>

using namespace bipolar;
using namespace std::literals;

TEST(Function, Traits) {
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<Function<int(int)>>);
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<Function<int(int)>>);

    EXPECT_FALSE(std::is_copy_constructible_v<Function<int(int)>>);
    EXPECT_FALSE(std::is_copy_assignable_v<Function<int(int)>>);

    // insitu
    auto lam = [](int x) { return x; };
    EXPECT_TRUE(
        (std::is_nothrow_constructible_v<Function<int(int)>, decltype(lam)>));

    // heap alloc
    std::array<void*, 100> arr;
    auto lam2 = [arr](int x) { return x; };
    EXPECT_FALSE(
        (std::is_nothrow_constructible_v<Function<int(int)>, decltype(lam2)>));
}

TEST(Function, Simple) {
    int x = 1000;
    std::array<void*, 7> arr;

    Function<std::size_t(const std::string&)> member_func(
        std::mem_fn(&std::string::size));
    EXPECT_EQ(4, member_func("1234"s));

    // insitu
    Function<int(int)> f([&x](int v) { return x += v; });
    EXPECT_EQ(1005, f(5));
    EXPECT_EQ(1011, f(6));
    EXPECT_EQ(1018, f(7));

    const Function<int(int)> cf([&x](int v) { return x += v; });
    EXPECT_EQ(1023, cf(5));
    EXPECT_EQ(1029, cf(6));
    EXPECT_EQ(1036, cf(7));

    // heap alloc
    Function<int(int)> hf([arr, &x](int v) { return x += v; });
    EXPECT_EQ(1041, hf(5));
    EXPECT_EQ(1047, hf(6));
    EXPECT_EQ(1054, hf(7));

    const Function<int(int)> chf([arr, &x](int v) { return x += v; });
    EXPECT_EQ(1059, chf(5));
    EXPECT_EQ(1065, chf(6));
    EXPECT_EQ(1072, chf(7));

    Function<int(int)> empty;
    EXPECT_EQ(empty, nullptr);
    EXPECT_THROW(empty(0), std::bad_function_call);

    Function<int(int)> add1([](int x) { return ++x; });
    swap(empty, add1);
    EXPECT_NE(empty, nullptr);
    EXPECT_EQ(44, empty(43));
}

TEST(Function, FunctionPtr) {
    int (*funcptr)(int) = [](int v) { return v * v; };

    // NOTE: mis-use Function
    Function<int(int)> f(funcptr);
    EXPECT_EQ(100, f(10));
    EXPECT_EQ(121, f(11));

    // NOTE: `funcptr` is moved before, but we know `funcptr` is trivial
    // user should avoid
    const Function<int(int)> cf(funcptr);
    EXPECT_EQ(100, cf(10));
    EXPECT_EQ(121, cf(11));
}

TEST(Function, OverloadedFunctor) {
    struct OverloadedFunctor {
        // variant 1
        int operator()(int x) {
            return 100 + 1 * x;
        }

        // variant 2 (const-overload of v1)
        int operator()(int x) const {
            return 100 + 2 * x;
        }

        // variant 3
        int operator()(int x, int) {
            return 100 + 3 * x;
        }

        // variant 4 (const-overload of v3)
        int operator()(int x, int) const {
            return 100 + 4 * x;
        }

        // variant 5 (non-const, has no const-overload)
        int operator()(int x, char const*) {
            return 100 + 5 * x;
        }

        // variant 6 (const only)
        int operator()(int x, std::vector<int> const&) const {
            return 100 + 6 * x;
        }
    };
    OverloadedFunctor of;
    auto const& cof = of;

    // mis-use Function, see NOTE in `FunctionPtr` test
    Function<int(int)> variant1(of);
    EXPECT_EQ(100 + 1 * 15, variant1(15));
    Function<int(int)> const cvariant1(of);
    EXPECT_EQ(100 + 1 * 15, cvariant1(15));

    Function<int(int)> variant2(cof);
    EXPECT_EQ(100 + 2 * 16, variant2(16));
    Function<int(int)> const cvariant2(cof);
    EXPECT_EQ(100 + 2 * 16, cvariant2(16));

    Function<int(int, int)> variant3(of);
    EXPECT_EQ(100 + 3 * 17, variant3(17, 0));
    Function<int(int, int)> const cvariant3(of);
    EXPECT_EQ(100 + 3 * 17, cvariant3(17, 0));

    Function<int(int, int)> variant4(cof);
    EXPECT_EQ(100 + 4 * 18, variant4(18, 0));
    Function<int(int, int)> const cvariant4(cof);
    EXPECT_EQ(100 + 4 * 18, cvariant4(18, 0));

    Function<int(int, char const*)> variant5(of);
    EXPECT_EQ(100 + 5 * 19, variant5(19, "foo"));
    Function<int(int, char const*)> const cvariant5(of);
    EXPECT_EQ(100 + 5 * 19, cvariant5(19, "foo"));

    Function<int(int, std::vector<int> const&)> variant6(of);
    EXPECT_EQ(100 + 6 * 20, variant6(20, {}));
    EXPECT_EQ(100 + 6 * 20, variant6(20, {1, 2, 3}));
    Function<int(int, std::vector<int> const&)> const cvariant6(of);
    EXPECT_EQ(100 + 6 * 20, cvariant6(20, {}));
    EXPECT_EQ(100 + 6 * 20, cvariant6(20, {1, 2, 3}));

    Function<int(int, std::vector<int> const&)> variant6const(cof);
    EXPECT_EQ(100 + 6 * 21, variant6const(21, {}));
    Function<int(int, std::vector<int> const&)> const cvariant6const(cof);
    EXPECT_EQ(100 + 6 * 21, cvariant6const(21, {}));
}

TEST(Function, Compare) {
    Function<int(int)> f;
    EXPECT_EQ(f, nullptr);

    Function<int(int)> add1;
    add1 = [](int x) { return x + 1; };
    EXPECT_NE(add1, nullptr);

    f = std::move(add1);
    EXPECT_NE(f, nullptr);
    EXPECT_EQ(f(10), 11);
}
