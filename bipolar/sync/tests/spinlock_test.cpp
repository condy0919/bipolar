#include "bipolar/sync/spinlock.hpp"

#include <gtest/gtest.h>

using namespace bipolar;

TEST(SpinLock, try_lock) {
    ASSERT_DEATH(
        {
            SpinLock lock;

            EXPECT_TRUE(lock.try_lock());
        },
        "SpinLock.*Assertion `locked_ == 0' failed");
}

TEST(SpinLock, lock) {
    SpinLock lock;

    lock.lock();
    lock.unlock();
}

TEST(SpinLock, unlock) {
    SpinLock lock;

    // does nothing
    lock.unlock();
}
