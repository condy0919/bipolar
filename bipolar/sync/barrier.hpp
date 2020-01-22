//!
//!
//!

#ifndef BIPOLAR_SYNC_BARRIER_HPP_
#define BIPOLAR_SYNC_BARRIER_HPP_

#include <pthread.h>

#include <cassert>

#include <boost/noncopyable.hpp>

namespace bipolar {
class Barrier final : public boost::noncopyable {
public:
    Barrier(unsigned n) noexcept {
        const int r = ::pthread_barrier_init(&barrier_, nullptr, n);
        assert(r == 0);
    }

    void wait() noexcept {
        ::pthread_barrier_wait(&barrier_);
    }

    ~Barrier() noexcept {
        ::pthread_barrier_destroy(&barrier_);
    }

private:
    ::pthread_barrier_t barrier_;
};
} // namespace bipolar

#endif
