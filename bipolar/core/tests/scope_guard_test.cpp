#include "bipolar/core/scope_guard.hpp"

#include <gtest/gtest.h>

using namespace bipolar;

TEST(ScopeGuard, OnExit) {
    EXPECT_THROW(
        [] {
            auto guard = ScopeGuardExit(
                [] { EXPECT_EQ(1, std::uncaught_exceptions()); });

            throw 0;
        }(),
        int);

    EXPECT_THROW(
        [] {
            auto guard = ScopeGuardExit([] { EXPECT_TRUE(false); });

            guard.dismiss();
            throw 0;
        }(),
        int);
}

TEST(ScopeGuard, OnSuccss) {
    EXPECT_NO_THROW([] {
        const int prev_uncaught_exceptions = std::uncaught_exceptions();
        auto guard = ScopeGuardSuccess([prev_uncaught_exceptions] {
            const int latest_uncaught_exceptions = std::uncaught_exceptions();
            EXPECT_EQ(prev_uncaught_exceptions, latest_uncaught_exceptions);
        });
    }());

    EXPECT_THROW(
        [] {
            auto gaurd = ScopeGuardSuccess([] { EXPECT_TRUE(false); });

            throw 0;
        }(),
        int);
}

TEST(ScopeGuard, OnFailure) {
    EXPECT_THROW(
        [] {
            const int prev_uncaught_exceptions = std::uncaught_exceptions();
            auto guard = ScopeGuardFailure([prev_uncaught_exceptions] {
                const int latest_uncaught_exceptions =
                    std::uncaught_exceptions();
                EXPECT_GT(latest_uncaught_exceptions, prev_uncaught_exceptions);
            });

            throw 0;
        }(),
        int);

    EXPECT_NO_THROW(
        [] { auto guard = ScopeGuardFailure([] { EXPECT_TRUE(false); }); }());
}
