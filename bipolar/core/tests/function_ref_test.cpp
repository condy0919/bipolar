#include "bipolar/core/function_ref.hpp"

#include <string>

#include <gtest/gtest.h>

using namespace bipolar;

TEST(FunctionRef, Traits) {
    EXPECT_TRUE(std::is_literal_type_v<FunctionRef<int(int)>>);

    EXPECT_TRUE(std::is_trivially_copy_constructible_v<FunctionRef<int(int)>>);
    EXPECT_TRUE(std::is_trivially_move_constructible_v<FunctionRef<int(int)>>);
    EXPECT_TRUE((std::is_trivially_constructible_v<FunctionRef<int(int)>,
                                                   FunctionRef<int(int)>&>));

    EXPECT_TRUE(std::is_trivially_copy_assignable_v<FunctionRef<int(int)>>);
    EXPECT_TRUE(std::is_trivially_move_assignable_v<FunctionRef<int(int)>>);
    EXPECT_TRUE((std::is_trivially_assignable_v<FunctionRef<int(int)>,
                                                FunctionRef<int(int)>&>));
    
    EXPECT_TRUE(std::is_nothrow_copy_constructible_v<FunctionRef<int(int)>>);
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<FunctionRef<int(int)>>);
    EXPECT_TRUE((std::is_nothrow_constructible_v<FunctionRef<int(int)>,
                                                 FunctionRef<int(int)>&>));

    EXPECT_TRUE(std::is_nothrow_copy_assignable_v<FunctionRef<int(int)>>);
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<FunctionRef<int(int)>>);
    EXPECT_TRUE((std::is_nothrow_assignable_v<FunctionRef<int(int)>,
                                              FunctionRef<int(int)>&>));
}

TEST(FunctionRef, Simple) {
    int x = 1000;
    auto lambda = [&x](int v) { return x += v; };

    FunctionRef<int(int)> fref = lambda;
    EXPECT_EQ(1005, fref(5));
    EXPECT_EQ(1011, fref(6));
    EXPECT_EQ(1018, fref(7));

    FunctionRef<int(int)> const cfref = lambda;
    EXPECT_EQ(1023, cfref(5));
    EXPECT_EQ(1029, cfref(6));
    EXPECT_EQ(1036, cfref(7));

    auto const& clambda = lambda;

    FunctionRef<int(int)> fcref = clambda;
    EXPECT_EQ(1041, fcref(5));
    EXPECT_EQ(1047, fcref(6));
    EXPECT_EQ(1054, fcref(7));

    FunctionRef<int(int)> const cfcref = clambda;
    EXPECT_EQ(1059, cfcref(5));
    EXPECT_EQ(1065, cfcref(6));
    EXPECT_EQ(1072, cfcref(7));
}

TEST(FunctionRef, FunctionPtr) {
    int (*funcptr)(int) = [](int v) { return v * v; };

    FunctionRef<int(int)> fref = funcptr;
    EXPECT_EQ(100, fref(10));
    EXPECT_EQ(121, fref(11));

    FunctionRef<int(int)> const cfref = funcptr;
    EXPECT_EQ(100, cfref(10));
    EXPECT_EQ(121, cfref(11));
}

TEST(FunctionRef, OverloadedFunctor) {
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

    FunctionRef<int(int)> variant1 = of;
    EXPECT_EQ(100 + 1 * 15, variant1(15));
    FunctionRef<int(int)> const cvariant1 = of;
    EXPECT_EQ(100 + 1 * 15, cvariant1(15));

    FunctionRef<int(int)> variant2 = cof;
    EXPECT_EQ(100 + 2 * 16, variant2(16));
    FunctionRef<int(int)> const cvariant2 = cof;
    EXPECT_EQ(100 + 2 * 16, cvariant2(16));

    FunctionRef<int(int, int)> variant3 = of;
    EXPECT_EQ(100 + 3 * 17, variant3(17, 0));
    FunctionRef<int(int, int)> const cvariant3 = of;
    EXPECT_EQ(100 + 3 * 17, cvariant3(17, 0));

    FunctionRef<int(int, int)> variant4 = cof;
    EXPECT_EQ(100 + 4 * 18, variant4(18, 0));
    FunctionRef<int(int, int)> const cvariant4 = cof;
    EXPECT_EQ(100 + 4 * 18, cvariant4(18, 0));

    FunctionRef<int(int, char const*)> variant5 = of;
    EXPECT_EQ(100 + 5 * 19, variant5(19, "foo"));
    FunctionRef<int(int, char const*)> const cvariant5 = of;
    EXPECT_EQ(100 + 5 * 19, cvariant5(19, "foo"));

    FunctionRef<int(int, std::vector<int> const&)> variant6 = of;
    EXPECT_EQ(100 + 6 * 20, variant6(20, {}));
    EXPECT_EQ(100 + 6 * 20, variant6(20, {1, 2, 3}));
    FunctionRef<int(int, std::vector<int> const&)> const cvariant6 = of;
    EXPECT_EQ(100 + 6 * 20, cvariant6(20, {}));
    EXPECT_EQ(100 + 6 * 20, cvariant6(20, {1, 2, 3}));

    FunctionRef<int(int, std::vector<int> const&)> variant6const = cof;
    EXPECT_EQ(100 + 6 * 21, variant6const(21, {}));
    FunctionRef<int(int, std::vector<int> const&)> const cvariant6const = cof;
    EXPECT_EQ(100 + 6 * 21, cvariant6const(21, {}));
}
