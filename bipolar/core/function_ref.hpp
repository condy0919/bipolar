//! FunctionRef
//!
//! # Brief
//!
//! `FunctionRef` is an efficient, type-erasing, non-owning reference to
//! a callable. This is intended for use as the type of a function parameter
//! that is not used after the function in question returns.
//!
//! This class does not own the callable, so it is not in general safe to store
//! a `FunctionRef`.
//!
//! see `FunctionRef` [`Proposal`] for details
//!
//! # Examples
//!
//! A basic usage
//!
//! ```
//! const int k = 13;
//! const auto l = [k](int x) { return x + k; };
//! FunctionRef<int(int)> addk = l;
//!
//! assert(addk(20) == 33);
//! assert(addk(10) == 23);
//! ```
//!
//! [`Proposal`]: http://open-std.org/JTC1/SC22/WG21/docs/papers/2018/p0792r2.html
//!

#ifndef BIPOLAR_CORE_FUNCTION_REF_HPP_
#define BIPOLAR_CORE_FUNCTION_REF_HPP_

#include <functional>
#include <memory>
#include <type_traits>

namespace bipolar {
template <typename>
class FunctionRef;

template <typename R, typename... Args>
class FunctionRef<R(Args...)> {
public:
    template <typename F,
              std::enable_if_t<
                  !std::is_same_v<std::remove_cv_t<std::remove_reference_t<F>>,
                                  FunctionRef> &&
                      std::is_invocable_r_v<R, F, Args...>,
                  int> = 0>
    // it has been checked by `std::enable_if_t`
    // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
    constexpr /*implicit*/ FunctionRef(F&& f) noexcept
        : obj_((void*)std::addressof(f)), call_(make_call<F>()) {}

    constexpr FunctionRef(const FunctionRef&) noexcept = default;

    template <typename F,
              std::enable_if_t<
                  !std::is_same_v<std::remove_cv_t<std::remove_reference_t<F>>,
                                  FunctionRef> &&
                      std::is_invocable_r_v<R, F, Args...>,
                  int> = 0>
    constexpr FunctionRef& operator=(F&& f) noexcept {
        obj_ = (void*)std::addressof(f);
        call_ = make_call<F>();
        return *this;
    }

    constexpr FunctionRef& operator=(const FunctionRef&) noexcept = default;

    R operator()(Args... args) const {
        return call_(obj_, std::forward<Args>(args)...);
    }

    constexpr void swap(FunctionRef& other) noexcept {
        std::swap(obj_, other.obj_);
        std::swap(call_, other.call_);
    }

private:
    using CallType = R (*)(void*, Args...);

    template <typename F>
    constexpr CallType make_call() noexcept {
        return [](void* obj, Args... args) -> R {
            return std::invoke(*static_cast<std::add_pointer_t<F>>(obj),
                               std::forward<Args>(args)...);
        };
    }

    void* obj_;
    CallType call_;
};

template <typename T>
inline constexpr void swap(FunctionRef<T>& lhs, FunctionRef<T>& rhs) noexcept {
    lhs.swap(rhs);
}
} // namespace bipolar

#endif
