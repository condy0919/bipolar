/// \file io_uring.cpp

#include "bipolar/io/io_uring.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <cassert>
#include <system_error>


namespace bipolar {
IOUringSQ::IOUringSQ(int fd, const struct io_uring_params* p) {
    assert(p);

    ring_sz_ = p->sq_off.array + p->sq_entries * sizeof(std::uint32_t);
    ring_ptr_ = mmap(0, ring_sz_, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_POPULATE, fd, IORING_OFF_SQ_RING);
    if (ring_ptr_ == MAP_FAILED) {
        throw std::system_error(errno, std::system_category());
    }

    char* ptr = (char*)ring_ptr_;
    khead_ = (std::uint32_t*)(ptr + p->sq_off.head);
    ktail_ = (std::uint32_t*)(ptr + p->sq_off.tail);
    kring_mask_ = (std::uint32_t*)(ptr + p->sq_off.ring_mask);
    kring_entries_ = (std::uint32_t*)(ptr + p->sq_off.ring_entries);
    kflags_ = (std::uint32_t*)(ptr + p->sq_off.flags);
    kdropped_ = (std::uint32_t*)(ptr + p->sq_off.dropped);
    array_ = (std::uint32_t*)(ptr + p->sq_off.array);

    const std::size_t size = p->sq_entries * sizeof(IOUringSQE);
    sqe_head_ = sqe_tail_ = 0;
    sqes_ = (IOUringSQE*)mmap(0, size, PROT_READ | PROT_WRITE,
                              MAP_SHARED | MAP_POPULATE, fd, IORING_OFF_SQES);
    if (sqes_ == MAP_FAILED) {
        const int err = errno;
        munmap(ring_ptr_, ring_sz_);
        throw std::system_error(err, std::system_category());
    }
}

IOUringSQ::~IOUringSQ() {
    munmap(sqes_, *kring_entries_ * sizeof(IOUringSQE));
    munmap(ring_ptr_, ring_sz_);
}

IOUringCQ::IOUringCQ(int fd, const struct io_uring_params* p) {
    assert(p);

    ring_sz_ = p->cq_off.cqes + p->cq_entries * sizeof(IOUringCQE);
    ring_ptr_ = mmap(0, ring_sz_, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_POPULATE, fd, IORING_OFF_CQ_RING);
    if (ring_ptr_ == MAP_FAILED) {
        throw std::system_error(errno, std::system_category());
    }

    char* ptr = (char*)ring_ptr_;
    khead_ = (std::uint32_t*)(ptr + p->cq_off.head);
    ktail_ = (std::uint32_t*)(ptr + p->cq_off.tail);
    kring_mask_ = (std::uint32_t*)(ptr + p->cq_off.ring_mask);
    kring_entries_ = (std::uint32_t*)(ptr + p->cq_off.ring_entries);
    koverflow_ = (std::uint32_t*)(ptr + p->cq_off.overflow);
    cqes_ = (IOUringCQE*)(ptr + p->cq_off.cqes);
}

IOUringCQ::~IOUringCQ() {
    munmap(ring_ptr_, ring_sz_);
}

IOUring::IOUring(unsigned entries, struct io_uring_params* p)
    : ring_fd_(io_uring_setup(entries, (assert(p), p))), // a tricky assert
      flags_(p->flags), sq_(ring_fd_, p), cq_(ring_fd_, p) {}

IOUring::~IOUring() {
    close(ring_fd_);
}

Result<Void, int> IOUring::register_buffer(const struct iovec* iovecs,
                                           std::size_t n) {
    if (io_uring_register(ring_fd_, IORING_REGISTER_BUFFERS, iovecs, n) < 0) {
        return Err(errno);
    }
    return Ok(Void{});
}

Result<Void, int> IOUring::unregister_buffer() {
    if (io_uring_register(ring_fd_, IORING_UNREGISTER_BUFFERS, NULL, 0) < 0) {
        return Err(errno);
    }
    return Ok(Void{});
}

Result<Void, int> IOUring::register_files(const int *files, std::size_t n) {
    if (io_uring_register(ring_fd_, IORING_REGISTER_FILES, files, n) < 0) {
        return Err(errno);
    }
    return Ok(Void{});
}

Result<Void, int> IOUring::unregister_files() {
    if (io_uring_register(ring_fd_, IORING_UNREGISTER_FILES, NULL, 0) < 0) {
        return Err(errno);
    }
    return Ok(Void{});
}

Result<Void, int> IOUring::register_eventfd(int evfd) {
    if (io_uring_register(ring_fd_, IORING_REGISTER_EVENTFD, &evfd, 1) < 0) {
        return Err(errno);
    }
    return Ok(Void{});
}

Result<Void, int> IOUring::unregister_eventfd() {
    if (io_uring_register(ring_fd_, IORING_UNREGISTER_EVENTFD, NULL, 0) < 0) {
        return Err(errno);
    }
    return Ok(Void{});
}

Result<std::reference_wrapper<IOUringSQE>, Void>
IOUring::get_submission_entry() {
    const std::uint32_t next = sq_.sqe_tail_ + 1;
    if (next - sq_.sqe_head_ > *sq_.kring_entries_) {
        return Err(Void{});
    }

    IOUringSQE& sqe = sq_.sqes_[sq_.sqe_tail_ & *sq_.kring_mask_];
    sq_.sqe_tail_ = next;
    return Ok(std::ref(sqe));
}

Result<std::reference_wrapper<IOUringCQE>, int>
IOUring::get_completion_entry(bool wait) {
    for (;;) {
        const std::uint32_t head = *cq_.khead_;

        if (__atomic_load_n(cq_.ktail_, __ATOMIC_ACQUIRE) != head) {
            return Ok(std::ref(cq_.cqes_[head & *cq_.kring_mask_]));
        }

        if (!wait) {
            return Err(EAGAIN);
        }

        if (io_uring_enter(ring_fd_, 0, 1, IORING_ENTER_GETEVENTS, NULL) < 0) {
            return Err(errno);
        }
    }
}

Result<int, int> IOUring::submit(std::size_t wait) {
    if (sq_.sqe_head_ == sq_.sqe_tail_) {
        return Ok(0);
    }

    // Fill in SQEs that we have queued up, adding them to the kernel ring
    const std::uint32_t mask = *sq_.kring_mask_;
    std::uint32_t to_submit = sq_.sqe_tail_ - sq_.sqe_head_;
    std::uint32_t submitted = 0;
    std::uint32_t ktail = *sq_.ktail_;
    while (to_submit--) {
        sq_.array_[ktail & mask] = sq_.sqe_head_ & mask;
        ++ktail;
        ++sq_.sqe_head_;
        ++submitted;
    }

    if (!submitted) {
        return Ok(0);
    }

    // Ensure that kernel sees the SQE updates before it sees the tail update
    __atomic_store_n(sq_.ktail_, ktail, __ATOMIC_RELEASE);

    if (unsigned flags = 0; wait || needs_enter(flags)) {
        if (wait) {
            if (wait > submitted) {
                wait = submitted;
            }
            flags |= IORING_ENTER_GETEVENTS;
        }

        if (io_uring_enter(ring_fd_, submitted, wait, flags, NULL) < 0) {
            return Err(errno);
        }
    }
    return Ok(static_cast<int>(submitted));
}

} // namespace bipolar
