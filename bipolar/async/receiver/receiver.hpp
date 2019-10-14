//! The `Receiver` type
//!
//!

#ifndef BIPOLAR_ASYNC_RECEIVER_RECEIVER_HPP_
#define BIPOLAR_ASYNC_RECEIVER_RECEIVER_HPP_

#include <utility>
#include <exception>
#include <type_traits>

#include "bipolar/async/receiver/traits.hpp"
#include "bipolar/async/receiver/callback.hpp"

namespace bipolar {
// TODO
template <typename E, typename... Vs>
class AnyReceiver : public PropertySet<BIPOLAR_ASYNC_PROPERTY(Receiver)> {
public:
    constexpr AnyReceiver() = default;

    constexpr AnyReceiver(AnyReceiver&& rhs) noexcept {
    }

    ~AnyReceiver() {

    }

    constexpr AnyReceiver& operator=(AnyReceiver&& rhs) noexcept {
        return *this;
    }

    template <typename... Args>
    constexpr void value(Args&&... args) {

    }

    template <typename T>
    constexpr void error(T&& e) noexcept {
        if (!done_) {
            done_ = true;

        }
    }

    constexpr void done() noexcept {
        if (!done_) {
            done_ = true;
        }
    }

private:


private:
    bool done_;
};

// forward
template <typename...>
class Receiver;

template <typename ValueFunc, typename ErrorFunc, typename DoneFunc>
class Receiver<ValueFunc, ErrorFunc, DoneFunc>
    : public PropertySet<BIPOLAR_ASYNC_PROPERTY(Receiver)> {
public:
    static_assert(
        std::is_nothrow_invocable_v<ErrorFunc, std::exception_ptr>,
        "error function must be noexcept and support std::exception_ptr");

    constexpr Receiver() : Receiver(ValueFunc{}, ErrorFunc{}, DoneFunc{}) {}

    constexpr Receiver(ValueFunc vf, ErrorFunc ef, DoneFunc df)
        : done_(false), vf_(std::move(vf)), ef_(std::move(ef)),
          df_(std::move(df)) {}

    template <typename... Args>
    constexpr void value(Args&&... args) noexcept(
        std::is_nothrow_invocable_v<ValueFunc, Args...>) {
        if (!done_) {
            vf_(std::forward<Args>(args)...);
        }
    }

    template <typename E>
    constexpr void error(E&& e) noexcept {
        static_assert(std::is_nothrow_invocable_v<ErrorFunc, E>,
                      "error function must be noexcept");
        if (!done_) {
            done_ = true;
            ef_(std::forward<E>(e));
        }
    }

    constexpr void done() noexcept(std::is_nothrow_invocable_v<DoneFunc>) {
        if (!done_) {
            done_ = true;
            df_();
        }
    }

private:
    bool done_;

    ValueFunc vf_;
    ErrorFunc ef_;
    DoneFunc df_;
};

template <typename R, typename ValueFunc, typename ErrorFunc, typename DoneFunc>
class Receiver<R, ValueFunc, ErrorFunc, DoneFunc>
    : public property_set_from_t<R> {
public:
    static_assert(!is_receiver_v<R>, "R must be a Receiver");

    static_assert(
        std::is_nothrow_invocable_v<ErrorFunc, R&, std::exception_ptr>,
        "error function must be noexcept and support std::exception_ptr");

    constexpr explicit Receiver(R recvr)
        : Receiver(std::move(recvr), ValueFunc{}, ErrorFunc{}, DoneFunc{}) {}

    constexpr Receiver(R recvr, ValueFunc vf, ErrorFunc ef, DoneFunc df)
        : done_(false), recvr_(std::move(recvr)), vf_(std::move(vf)),
          ef_(std::move(ef)), df_(std::move(df)) {}

    template <typename... Args>
    constexpr void value(Args&&... args) noexcept(
        std::is_nothrow_invocable_v<ValueFunc, R&, Args...>) {
        if (!done_) {
            vf_(recvr_, std::forward<Args>(args)...);
        }
    }

    template <typename E>
    constexpr void error(E&& e) noexcept {
        static_assert(std::is_nothrow_invocable_v<ErrorFunc, R&, E>,
                      "error function must be noexcept");
        if (!done_) {
            done_ = true;
            ef_(recvr_, std::forward<E>(e));
        }
    }

    constexpr void done() noexcept(std::is_nothrow_invocable_v<DoneFunc>()) {
        if (!done_) {
            done_ = true;
            df_(recvr_);
        }
    }

    constexpr R& get_receiver() & noexcept {
        return recvr_;
    }

    constexpr const R& get_receiver() const& noexcept {
        return recvr_;
    }

    constexpr R&& get_receiver() && noexcept {
        return std::move(recvr_);
    }

private:
    bool done_;

    R recvr_;

    ValueFunc vf_;
    ErrorFunc ef_;
    DoneFunc df_;
};

template <>
class Receiver<> : public Receiver<Ignore, Abort, Ignore> {
public:
    using Receiver<Ignore, Abort, Ignore>::Receiver;
};

////////////////////////////////////////////////////////////////////////////////
// deduction guides for Receiver
////////////////////////////////////////////////////////////////////////////////

Receiver() -> Receiver<>;

template <typename ValueFunc, typename ErrorFunc, typename DoneFunc>
Receiver(ValueFunc, ErrorFunc, DoneFunc)
    -> Receiver<ValueFunc, ErrorFunc, DoneFunc>;

template <typename R>
Receiver(R) -> Receiver<R, SetValue, SetError, SetDone>;

template <typename R, typename ValueFunc, typename ErrorFunc, typename DoneFunc>
Receiver(R, ValueFunc, ErrorFunc, DoneFunc)
    -> Receiver<R, ValueFunc, ErrorFunc, DoneFunc>;

} // namespace bipolar

#endif
