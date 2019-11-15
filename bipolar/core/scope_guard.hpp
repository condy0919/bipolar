//! `ScopeGuard`s
//!
//! There are three `ScopeGuard`s:
//! - `ScopeGuardExit`
//!    invokes when goes out of scope unless dismissed
//! - `ScopeGuardSuccess`
//!    invokes when no exception thrown
//! - `ScopeGuardFailure`
//!    invokes when a new exception thrown
//!

#ifndef BIPOLAR_CORE_SCOPE_GUARD_HPP_
#define BIPOLAR_CORE_SCOPE_GUARD_HPP_

#include <exception>
#include <type_traits>
#include <utility>

namespace bipolar {
namespace detail {
template <typename F, std::enable_if_t<std::is_invocable_v<F>, int> = 0>
class ScopeGuardBase {
public:
    constexpr ScopeGuardBase(F&& f) noexcept(
        std::is_nothrow_move_constructible_v<F>)
        : f_(std::move(f)) {}

    constexpr ScopeGuardBase(const F& f) noexcept(
        std::is_nothrow_copy_constructible_v<F>)
        : f_(f) {}

    ScopeGuardBase(ScopeGuardBase&&) = delete;
    ScopeGuardBase(const ScopeGuardBase&) = delete;

    ScopeGuardBase& operator=(ScopeGuardBase&&) = delete;
    ScopeGuardBase& operator=(const ScopeGuardBase&) = delete;

protected:
    constexpr void operator()() noexcept(std::is_nothrow_invocable_v<F>) {
        f_();
    }

private:
    F f_;
};
} // namespace detail

/// ScopeGuardExit
///
/// # Brief
///
/// `ScopeGuardExit` is a deferred functor wrapper.
/// It's neither copyable, movable or assignable.
/// It will invoke the functor when it goes out of scope unless dismissed.
///
/// # Examples
///
/// a dismiss use:
///
/// ```
/// void insert(Key k, Value v) {
///     // map::insert may throw std::bad_alloc
///     auto [it, inserted] = k2v.insert(std::make_pair(k, v));
///     auto guard = ScopeGuardExit([it, &k2v] {
///         k2v.erase(it);
///     });
///
///     // if the following insert method throws a std::bad_alloc,
///     // the `ScopeGuardExit` object will make k2v recover to the previous
///     // consistant state
///     v2k.insert(std::make_pair(k, v));
///
///     // no exception thrown, so we cancel the guard
///     guard.dismiss();
/// }
/// ```
///
/// a typical use:
///
/// ```
/// void test() {
///     int fd = open(); // a resource type without RAII semantics
///     auto guard = [fd] { close(fd); };
///
///     if (fd < 0) {
///         return;
///     }
///
///     int ret = recv(fd);
///     if (ret < 0) {
///         return;
///     }
///
///     // ...
/// }
/// ```
template <typename F>
class ScopeGuardExit : public detail::ScopeGuardBase<F> {
public:
    using detail::ScopeGuardBase<F>::ScopeGuardBase;

    ~ScopeGuardExit() noexcept(std::is_nothrow_invocable_v<F>) {
        if (!dismiss_) {
            (*this)();
        }
    }

    void dismiss() noexcept {
        dismiss_ = true;
    }

private:
    bool dismiss_ = false;
};

template <typename F>
ScopeGuardExit(F&&) -> ScopeGuardExit<std::decay_t<F>>;

/// ScopeGuardSuccess
///
/// # Brief
///
/// `ScopeGuardSuccess` is similar to `ScopeGuardExit` while it's invoked
/// only when no new exception thrown.
///
/// # Examples
///
/// ```
/// void test() {
///     // it never puts
///     auto guard = ScopeGuardSuccess([] { puts("no exception thrown"); });
///
///     throw std::runtime_error();
/// }
/// ```
template <typename F>
class ScopeGuardSuccess : public detail::ScopeGuardBase<F> {
public:
    using detail::ScopeGuardBase<F>::ScopeGuardBase;

    ~ScopeGuardSuccess() noexcept(std::is_nothrow_invocable_v<F>) {
        const int latest_uncaught_exceptions = std::uncaught_exceptions();
        if (latest_uncaught_exceptions == prev_uncaught_exceptions) {
            (*this)();
        }
    }

private:
    const int prev_uncaught_exceptions = std::uncaught_exceptions();
};

template <typename F>
ScopeGuardSuccess(F&&) -> ScopeGuardSuccess<std::decay_t<F>>;

/// ScopeGuardFailure
///
/// # Brief
///
/// `ScopeGuardFailure` is similar to `ScopeGuardExit` while it's invoked
/// only when a new exception thrown.
///
/// # Examples
///
/// ```
/// void test() {
///     // it always puts
///     auto guard = ScopeGuardFailure([] { puts("exception thrown"); });
///
///     throw std::runtime_error();
/// }
/// ```
template <typename F>
class ScopeGuardFailure : public detail::ScopeGuardBase<F> {
public:
    using detail::ScopeGuardBase<F>::ScopeGuardBase;

    ~ScopeGuardFailure() noexcept(std::is_nothrow_invocable_v<F>) {
        const int latest_uncaught_exceptions = std::uncaught_exceptions();
        if (latest_uncaught_exceptions > prev_uncaught_exceptions) {
            (*this)();
        }
    }

private:
    const int prev_uncaught_exceptions = std::uncaught_exceptions();
};

template <typename F>
ScopeGuardFailure(F&&) -> ScopeGuardFailure<std::decay_t<F>>;

} // namespace bipolar

#endif
