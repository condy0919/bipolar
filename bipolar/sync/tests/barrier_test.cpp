#include "bipolar/sync/barrier.hpp"

#include <thread>

#include <gtest/gtest.h>

using namespace bipolar;

TEST(Barrier, init) {
    ASSERT_DEATH({ Barrier b(0); }, "Barrier.*Assertion `r == 0' failed");
}

TEST(Barrier, wait) {
    Barrier b(1);
    b.wait();
}

TEST(Barrier, all) {
    Barrier b(2);
    bool done = false;

    std::thread t([&b, &done]() {
        b.wait();
        done = true;
    });

    b.wait();
    if (t.joinable()) {
        t.join();
    }

    EXPECT_TRUE(done);
}
