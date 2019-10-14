// @todo use markdown as comment doc
/// \file io_uring.hpp
/// stole from http://git.kernel.dk/cgit/liburing/

#ifndef BIPOLAR_IO_IOURING_HPP_
#define BIPOLAR_IO_IOURING_HPP_

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
    /// \brief Vectored read
    ///
    /// \param fd target fd
    /// \param iovecs[] pointer to iovecs
    /// \param n number of iovecs
    /// \param offset offset into file
    void readv(int fd, const struct iovec iovecs[], std::size_t n,
               off_t offset) {
        prep_rw(IORING_OP_READV, fd, iovecs, n, offset);
    }

    /// \brief Fixed read
    /// \c buf should be \c register_buffer ed
    ///
    /// \param fd target fd
    /// \param buf buffer
    /// \param n buffer size
    /// \param offset offset into file
    /// \param buf_index index into fixed buffer
    void read_fixed(int fd, void* buf, std::size_t n, off_t offset,
                    std::uint16_t buf_index) {
        prep_rw(IORING_OP_READ_FIXED, fd, buf, n, offset);
        this->buf_index = buf_index;
    }

    /// \brief Vectored write
    ///
    /// \param fd target fd
    /// \param iovecs[] pointer to iovecs
    /// \param n number of iovecs
    /// \param offset offset into file
    void writev(int fd, const struct iovec iovecs[], std::size_t n,
                off_t offset) {
        prep_rw(IORING_OP_WRITEV, fd, iovecs, n, offset);
    }

    /// \brief Fixed write
    ///
    /// \param fd target fd
    /// \param buf buffer
    /// \param n buffer size
    /// \param offset offset into file
    /// \param buf_index index into fixed buffer
    void write_fixed(int fd, const void* buf, std::size_t n, off_t offset,
                     std::uint16_t buf_index) {
        prep_rw(IORING_OP_WRITE_FIXED, fd, buf, n, offset);
        this->buf_index = buf_index;
    }

    /// \brief Poll the \c fd
    /// \note It works like an \c epoll with \c EPOLLONESHOT
    ///
    /// \param fd target fd
    /// \param poll_events poll events
    void poll_add(int fd, std::uint16_t poll_events) {
        clear();
        this->opcode = IORING_OP_POLL_ADD;
        this->fd = fd;
        this->poll_events = poll_events;
    }

    /// \brief Remove an existing poll request by comparing \c user_data 
    ///
    /// \param user_data user data
    void poll_remove(void* user_data) {
        clear();
        this->opcode = IORING_OP_POLL_REMOVE;
        this->addr = (std::uint64_t)user_data;
    }

    /// \brief File sync
    /// \note IORING_FSYNC_DATASYNC makes it behave like \c fdatasync
    ///
    /// \param fd target fd
    /// \param fsync_flags fsync flags
    /// \see fsync
    void fsync(int fd, std::uint32_t fsync_flags) {
        clear();
        this->opcode = IORING_OP_FSYNC;
        this->fd = fd;
        this->fsync_flags = fsync_flags;
    }

    /// \brief Sync file range
    ///
    /// \param fd target fd
    /// \param offset offset into file
    /// \param nbytes range length
    /// \param flags flags
    void sync_file_range(int fd, off_t offset, off_t nbytes,
                         std::uint32_t flags) {
        clear();
        this->opcode = IORING_OP_SYNC_FILE_RANGE;
        this->fd = fd;
        this->off = offset;
        this->len = nbytes;
        this->sync_range_flags = flags;
    }

    /// \brief Recv msg
    ///
    /// \param fd target fd
    /// \param msgs[] pointer to msgs
    /// \param n msgs size
    void recvmsg(int fd, struct msghdr msgs[], std::size_t n) {
        clear();
        this->opcode = IORING_OP_RECVMSG;
        this->fd = fd;
        this->addr = (std::uint64_t)msgs;
        this->len = n;
    }

    /// \brief Send msg
    ///
    /// \param fd target fd
    /// \param msgs[] pointer to msgs
    /// \param n msgs size
    void sendmsg(int fd, const struct msghdr msgs[], std::size_t n) {
        clear();
        this->opcode = IORING_OP_SENDMSG;
        this->fd = fd;
        this->addr = (std::uint64_t)msgs;
        this->len = n;
    }

    /// \brief Don't perform any I/O
    void nop() {
        clear();
        this->opcode = IORING_OP_NOP;
    }

    /// \brief Clear
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
struct IOUringCQE : io_uring_cqe {};

/// \class IOUringSQ
/// \brief IO submission queue
/// \see io_sq_ring in
/// https://elixir.bootlin.com/linux/latest/source/fs/io_uring.c
class IOUringSQ {
    friend class IOUring;
public:
    /// \brief Constructs a submission queue
    ///
    /// \param fd the ring fd returned by \c io_uring_setup
    /// \param p the params modified by \c io_uring_setup
    /// \throw std::system_error
    /// \see IOUring
    IOUringSQ(int fd, const struct io_uring_params* p);

    /// \brief Destructs a submission queue
    ~IOUringSQ();

private:
    std::uint32_t* khead_; ///< Head offset into the ring, kernel controlled
    std::uint32_t* ktail_; ///< Tail offset into the ring, user controlled
    std::uint32_t* kring_mask_;    ///< Constant, equals to kring_entries_ - 1
    std::uint32_t* kring_entries_; ///< Constant, power of 2
    std::uint32_t* kflags_;        ///< Runtime flags, kernel controlled
    std::uint32_t* kdropped_;      ///< Invalid entries dropped by kernel
    std::uint32_t* array_;         ///< Ring buffer of indices to SQEs

    IOUringSQE* sqes_;       ///< The SQEs ring
    std::uint32_t sqe_head_; ///< Head offset into the SQEs ring
    std::uint32_t sqe_tail_; ///< Tail offset into the SQEs ring

    std::size_t ring_sz_;
    void* ring_ptr_;
};

/// \class IOUringCQ
/// \brief IO completion queue
/// \see io_cq_ring in
/// https://elixir.bootlin.com/linux/latest/source/fs/io_uring.c
class IOUringCQ {
    friend class IOUring;
public:
    /// \brief Constructs a completion queue
    ///
    /// \param fd the ring fd returned by \c io_uring_setup
    /// \param p the params modified by \c io_uring_setup
    /// \throw std::system_error
    /// \see IOUring
    IOUringCQ(int fd, const struct io_uring_params* p);

    /// \brief Destructs a completion queue
    ~IOUringCQ();

private:
    std::uint32_t* khead_;      ///< Head offset into the ring, user controlled
    std::uint32_t* ktail_;      ///< Tail offset into the ring, kernel controlled
    std::uint32_t* kring_mask_; ///< Constant, equals to kring_entries_ - 1
    std::uint32_t* kring_entries_; ///< Constant, power of 2
    std::uint32_t* koverflow_;     ///< Number of overflowed completion events
    IOUringCQE* cqes_;             ///< Ring buffer of completion events

    std::size_t ring_sz_;
    void* ring_ptr_;
};

/// \class IOUring
/// \brief IO uring
class IOUring {
public:
    /// \brief Constructs a \c IOUring
    /// \note Only \c io_uring_params.flags, \c io_uring_params.sq_thread_cpu
    /// and \c io_uring_params.sq_thread_idle are user-configurable
    ///
    /// \param entries
    /// \param p
    /// \throw std::system_error
    /// \see io_uring_setup
    IOUring(unsigned entries, struct io_uring_params* p);

    /// \brief Destructs a \c IOUring
    ~IOUring();

    /// @{
    /// \brief Registers user buffer
    ///
    /// \param iovecs[] pointer to iovecs
    /// \param n iovecs size
    ///
    /// \return \c Err with errno on failure; \c Ok on success
    Result<Void, int> register_buffer(const struct iovec iovecs[],
                                      std::size_t n);
    Result<Void, int> unregister_buffer();
    /// @}

    /// @{
    /// \brief Registers user files
    ///
    /// \param files[] pointer to files
    /// \param n files size
    ///
    /// \return \c Err with errno on failure; \c Ok on success
    Result<Void, int> register_files(const int files[], std::size_t n);
    Result<Void, int> unregister_files();
    /// @}

    /// @{
    /// \brief Register eventfd
    ///
    /// \param event_fd eventfd
    ///
    /// \return \c Err with errno on failure; \c Ok on success
    Result<Void, int> register_eventfd(int event_fd);
    Result<Void, int> unregister_eventfd();
    /// @}

    /// \brief Return a SQE to fill.
    /// Application must later call \c submit when it's ready to tell the 
    /// kernel about it. The caller may call this function multiple times
    /// before calling \c submit
    ///
    /// \return an \c Ok with vacant SQE or \c Err Void
    /// \see submit
    Result<std::reference_wrapper<IOUringSQE>, Void> get_submission_entry();

    /// \brief Returns an IO CQE, if available.
    ///
    /// \param wait Will it wait until completion event available?
    ///
    /// \return an \c Ok with CQE or \c Err with errno
    Result<std::reference_wrapper<IOUringCQE>, int>
    get_completion_entry(bool wait = true);

    /// \brief Alias for NO_WAIT get_completion_entry
    /// \see get_completion_entry
    Result<std::reference_wrapper<IOUringCQE>, int> peek_completion_entry() {
        return get_completion_entry(/* wait = */ false);
    }

    /// \brief Submit SQEs acquired from \c get_submission_entry to the kernel
    /// If \c nr_wait > 0, allows waiting for events as well.
    /// Default behavisor is no wait.
    ///
    /// \param nr_wait 
    /// \return the number of SQEs submitted or \c errno
    Result<int, int> submit(std::size_t nr_wait = 0);

    /// \brief Advances completion ring head
    ///
    /// \param n advance length
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
