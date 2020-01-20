//! SpinLock pool
//!

#ifndef BIPOLAR_SYNC_SPINLOCK_POOL_HPP_
#define BIPOLAR_SYNC_SPINLOCK_POOL_HPP_

#include <atomic>
#include <cassert>
#include <cstdint>

#include "bipolar/core/hash.hpp"
#include "bipolar/sync/cacheline.hpp"
#include "bipolar/sync/spinlock.hpp"

#include <boost/noncopyable.hpp>

namespace bipolar {
/// Pool of spinlocks
///
template <std::size_t POOL_SIZE>
class SpinLockPool final : public boost::noncopyable {
public:
    /// Gets the lock for the given address
    SpinLock& lock_for(const void* p) noexcept {
        const std::size_t idx =
            fibhash<POOL_SIZE>(reinterpret_cast<std::intptr_t>(p));
        return spinlock_pool_[idx];
    }

private:
    alignas(BIPOLAR_CACHELINE_SIZE) SpinLock spinlock_pool_[POOL_SIZE];
};

} // namespace bipolar
