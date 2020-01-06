//! SpinLock
//!

#ifndef BIPOLAR_SYNC_SPINLOCK_HPP_
#define BIPOLAR_SYNC_SPINLOCK_HPP_

#include <atomic>
#include <cassert>
#include <cstdint>

#include "bipolar/core/thread_safety.hpp"

#include <boost/noncopyable.hpp>

namespace bipolar {
/// SpinLock
///
/// `xchg` is cheaper than `cmpxchg` on most platforms.
///
/// See [it](https://www.agner.org/optimize/instruction_tables.pdf) for details.
class BIPOLAR_CAPABILITY("mutex") SpinLock final : public boost::noncopyable {
public:
    constexpr SpinLock() noexcept : locked_(0) {}

    ~SpinLock() noexcept {
        assert(locked_ == 0);
    }

    /// Trys to acquire the lock
    bool try_lock() noexcept BIPOLAR_TRY_ACQUIRE() {
        return locked_.exchange(1, std::memory_order_acquire) == 0;
    }

    /// Acquires the lock
    void lock() noexcept BIPOLAR_ACQUIRE() {
        std::uint32_t wait_iters = 0;

        while (true) {
            if (try_lock()) {
                return;
            }

            do {
                // exponential backoff
                ++wait_iters;
                if (wait_iters > 10) {
                    wait_iters = 10;
                }

                for (std::uint32_t i = 0; i < (1 << wait_iters); ++i) {
                    __builtin_ia32_pause();
                }
            } while (locked_.load(std::memory_order_relaxed) != 0);
        }
    }

    /// Releases the lock
    void unlock() noexcept BIPOLAR_RELEASE() {
        locked_.store(0, std::memory_order_release);
    }

private:
    /// Zero means unlocked
    std::atomic_int_fast32_t locked_;
};
} // namespace bipolar

#endif
