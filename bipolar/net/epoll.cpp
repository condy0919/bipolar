#include "bipolar/net/epoll.hpp"

#include <unistd.h>

namespace bipolar {
Epoll::~Epoll() noexcept {
    if (epfd_ != -1) {
        // FIXME diagnose required
        ::close(epfd_);
    }
}

Result<Epoll, int> Epoll::try_clone() noexcept {
    const int new_fd = ::dup(epfd_);
    if (new_fd == -1) {
        return Err(errno);
    }
    return Ok(Epoll(new_fd));
}

Result<Epoll, int> Epoll::create() noexcept {
    const int epfd = ::epoll_create1(EPOLL_CLOEXEC);
    if (epfd == -1) {
        return Err(errno);
    }
    return Ok(Epoll(epfd));
}

Result<Void, int> Epoll::poll(std::vector<struct epoll_event>& events,
                              std::chrono::milliseconds timeout) noexcept {
    const int ret =
        ::epoll_wait(epfd_, events.data(), events.size(), timeout.count());
    if (ret == -1) {
        return Err(errno);
    }

    events.resize(static_cast<std::size_t>(ret));
    return Ok(Void{});
}

Result<Void, int> Epoll::add(int fd, void* data,
                             std::uint32_t interests) noexcept {
    struct epoll_event ev;
    ev.events = interests;
    ev.data.ptr = data;

    const int ret = ::epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(Void{});
}

Result<Void, int> Epoll::mod(int fd, void* data,
                             std::uint32_t interests) noexcept {
    struct epoll_event ev;
    ev.events = interests;
    ev.data.ptr = data;

    const int ret = ::epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(Void{});
}

Result<Void, int> Epoll::del(int fd) noexcept {
    const int ret = ::epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(Void{});
}
} // namespace bipolar
