#include "bipolar/net/epoll.hpp"

#include <unistd.h>

#include <array>
#include <tuple>

#include "bipolar/core/assert.hpp"

namespace bipolar {
Epoll::~Epoll() noexcept {
    if (epfd_ != -1) {
        const int ret = ::close(epfd_);
        BIPOLAR_ASSERT(ret == 0, "epoll fd closed with error: {}", ret);
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
        ::epoll_wait(epfd_, events.data(), events.capacity(), timeout.count());
    if (ret == -1) {
        return Err(errno);
    }

    events.resize(static_cast<std::size_t>(ret));
    return Ok(Void{});
}

Result<Void, int> Epoll::del(int fd) noexcept {
    const int ret = ::epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
    if (ret == -1) {
        return Err(errno);
    }
    return Ok(Void{});
}

std::string stringify_interests(int interests) {
    constexpr std::array<std::tuple<int, const char*>, 6> opt_tbl = {{
        {EPOLLIN, "IN"},
        {EPOLLOUT, "OUT"},
        {EPOLLRDHUP, "RDHUP"},
        {EPOLLPRI, "PRI"},
        {EPOLLERR, "ERR"},
        {EPOLLHUP, "HUP"},
    }};

    std::string ret;

    // IN OUT RDHUP PRI ERR HUP
    // 2 + 3 + 5 + 3 + 3 + 3 = 19
    // round (19 + additional 6 whitespaces) up to 32
    ret.reserve(32);
    for (auto opt : opt_tbl) {
        if (interests & std::get<0>(opt)) {
            ret.append(std::get<1>(opt));
            ret.append(1, ' ');
        }
    }

    return ret;
}
} // namespace bipolar
