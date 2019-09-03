#ifndef BIPOLAR_IO_IOURING_HPP_
#define BIPOLAR_IO_IOURING_HPP_

/// \file io_uring.hpp
/// stole from http://git.kernel.dk/cgit/liburing/

#include <csignal>
#include <cstring>
#include <cstdint>
#include <functional>

#include "bipolar/core/void.hpp"
#include "bipolar/core/option.hpp"
#include "bipolar/core/result.hpp"

#include "liburing.h"

namespace bipolar {
/// \struct IOUringSQE
/// \brief IO submission queue entry
struct IOUringSQE : io_uring_sqe {
    /// \brief 
    ///
    /// \param data
    void data(void* data) {
        this->user_data = (std::uint64_t)data;
    }

    /// \brief 
    ///
    /// \return 
    void* data() const {
        return (void*)(std::uintptr_t)this->user_data;
    }

    /// \brief 
    ///
    /// \param flag
    void flags(std::uint8_t flag) {
        this->io_uring_sqe::flags = flag;
    }

    /// \brief 
    ///
    /// \return 
    std::uint8_t flags() const {
        return this->io_uring_sqe::flags;
    }

    /// \brief 
    ///
    /// \param fd
    /// \param iovecs[]
    /// \param n
    /// \param offset
    void readv(int fd, const struct iovec iovecs[], std::size_t n,
               off_t offset) {
        prep_rw(IORING_OP_READV, fd, iovecs, n, offset);
    }

    /// \brief 
    ///
    /// \param fd
    /// \param buf
    /// \param n
    /// \param offset
    void read_fixed(int fd, void* buf, std::size_t n, off_t offset,
                    std::uint16_t buf_index) {
        prep_rw(IORING_OP_READ_FIXED, fd, buf, n, offset);
        this->buf_index = buf_index;
    }

    /// \brief 
    ///
    /// \param fd
    /// \param iovecs[]
    /// \param n
    /// \param offset
    void writev(int fd, const struct iovec iovecs[], std::size_t n,
                off_t offset) {
        prep_rw(IORING_OP_WRITEV, fd, iovecs, n, offset);
    }

    /// \brief 
    ///
    /// \param fd
    /// \param buf
    /// \param n
    /// \param offset
    void write_fixed(int fd, const void* buf, std::size_t n, off_t offset,
                     std::uint16_t buf_index) {
        prep_rw(IORING_OP_WRITE_FIXED, fd, buf, n, offset);
        this->buf_index = buf_index;
    }

    /// \brief 
    ///
    /// \param fd
    /// \param poll_mask
    void poll_add(int fd, std::uint16_t poll_mask) {
        clear();
        this->opcode = IORING_OP_POLL_ADD;
        this->fd = fd;
        this->poll_events = poll_mask;
    }

    /// \brief 
    ///
    /// \param user_data
    void poll_remove(void* user_data) {
        clear();
        this->opcode = IORING_OP_POLL_REMOVE;
        this->addr = (std::uint64_t)user_data;
    }

    /// \brief 
    ///
    /// \param fd
    /// \param fsync_flags
    void fsync(int fd, std::uint32_t fsync_flags) {
        clear();
        this->opcode = IORING_OP_FSYNC;
        this->fd = fd;
        this->fsync_flags = fsync_flags;
    }

    /// \brief 
    void nop() {
        clear();
        this->opcode = IORING_OP_NOP;
    }

    /// \brief 
    void clear() {
        std::memset(this, 0, sizeof(*this));
    }

private:
    void prep_rw(int op, int fd, const void* addr, std::size_t len,
                 off_t offset) {
        clear();
        this->opcode = op;
        this->fd = fd;
        this->off = offset;
        this->addr = (std::uint64_t)addr;
        this->len = len;
    }
};

/// \struct IOUringCQE
/// \brief IO completion queue entry
struct IOUringCQE : io_uring_cqe {
    /// \brief 
    ///
    /// \return 
    void* data() const {
        return (void*)(std::uintptr_t)this->user_data;
    }
};

/// \class IOUringSQ
/// \brief IO submission queue
class IOUringSQ {
    friend class IOUring;
public:
    /// \brief 
    ///
    /// \param fd
    /// \param p
    IOUringSQ(int fd, const struct io_uring_params* p);

    /// \brief 
    ~IOUringSQ();

private:
    std::uint32_t* khead_;
    std::uint32_t* ktail_;
    std::uint32_t* kring_mask_;
    std::uint32_t* kring_entries_;
    std::uint32_t* kflags_;
    std::uint32_t* kdropped_;
    std::uint32_t* array_;

    IOUringSQE* sqes_;
    std::uint32_t sqe_head_;
    std::uint32_t sqe_tail_;

    std::size_t ring_sz_;
    void* ring_ptr_;
};

/// \class IOUringCQ
/// \brief IO completion queue
class IOUringCQ {
    friend class IOUring;
public:
    /// \brief 
    ///
    /// \param fd
    /// \param p
    IOUringCQ(int fd, const struct io_uring_params* p);

    /// \brief 
    ~IOUringCQ();

private:
    std::uint32_t* khead_;
    std::uint32_t* ktail_;
    std::uint32_t* kring_mask_;
    std::uint32_t* kring_entries_;
    std::uint32_t* koverflow_;
    IOUringCQE* cqes_;

    std::size_t ring_sz_;
    void* ring_ptr_;
};

/// \class IOUring
/// \brief IO uring
class IOUring {
public:
    /// \brief 
    ///
    /// \param entries
    /// \param p
    IOUring(unsigned entries, struct io_uring_params* p);

    /// \brief 
    ~IOUring();

    /// @{
    /// \brief 
    ///
    /// \param iovecs[]
    /// \param n
    ///
    /// \return 
    Result<Void, int> register_buffer(const struct iovec iovecs[],
                                      std::size_t n);
    Result<Void, int> unregister_buffer();
    /// @}

    /// @{
    /// \brief 
    ///
    /// \param files
    /// \param n
    ///
    /// \return 
    Result<Void, int> register_files(const int* files, std::size_t n);
    Result<Void, int> unregister_files();
    /// @}

    /// @{
    /// \brief 
    ///
    /// \param event_fd
    ///
    /// \return 
    Result<Void, int> register_eventfd(int event_fd);
    Result<Void, int> unregister_eventfd();
    /// @}

    /// \brief Return a SQE to fill.
    /// Application must later call \c submit when it's ready to tell the 
    /// kernel about it. The caller may call this function multiple times
    /// before calling \c submit
    ///
    /// \return an \c Ok with vacant SQE, \c Err Void
    /// \see submit
    Result<std::reference_wrapper<IOUringSQE>, Void> get_submission_entry();

    /// \brief 
    ///
    /// \param wait
    ///
    /// \return
    Result<Option<std::reference_wrapper<IOUringCQE>>, int>
    get_completion_entry(bool wait = false);

    /// \brief Submit SQEs acquired from \c get_submission_entry to the kernel
    /// If \c wait > 0, allows waiting for events as well.
    /// Default behavisor is no wait.
    ///
    /// \param wait 
    /// \return the number of SQEs submitted or \c errno
    Result<int, int> submit(std::size_t wait = 0);

    /// \brief 
    ///
    /// \param cqe
    void seen(std::size_t n) {
        __atomic_add_fetch(cq_.khead_, n, __ATOMIC_RELEASE);
    }

private:
    // Returns true if we're not using SQ thread (thus nobody submits but us)
    // or if IORING_SQ_NEED_WAKEUP is set, so submit thread must be explicitly
    // awakened. For the latter case, we set the thread wakeup flag.
    bool needs_enter(unsigned& flags) {
        if (!(flags_ & IORING_SETUP_SQPOLL)) {
            return true;
        }
        if ((*sq_.kflags_ & IORING_SQ_NEED_WAKEUP)) {
            flags |= IORING_ENTER_SQ_WAKEUP;
            return true;
        }
        return false;
    }

private:
    int ring_fd_;
    std::uint32_t flags_;
    IOUringSQ sq_;
    IOUringCQ cq_;
};

} // namespace bipolar

#endif
