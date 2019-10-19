//! Try
//!
//! see `Try` type for details.
//!

#ifndef BIPOLAR_CORE_TRY_HPP_
#define BIPOLAR_CORE_TRY_HPP_

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace bipolar {
namespace detail {
// Reduces code duplication through a base class
class TryBase {
    static_assert(std::is_nothrow_move_constructible_v<std::exception_ptr>,
                  "std::exception_ptr's move ctor must be noexcept");
    static_assert(sizeof(std::exception_ptr) == sizeof(void*),
                  "std::exception_ptr not a pointer");

public:
    enum class State : std::uintptr_t {
        NOTHING = 0,
        VALUE = 1,
        EXCEPTION_MIN = 2, // or anything greater
    };

    union Storage {
        State state_;
        std::exception_ptr ex_;

        constexpr Storage() : state_(State::NOTHING) {}
        constexpr Storage(State st) : state_(st) {}
        Storage(std::exception_ptr&& ex) : ex_(std::move(ex)) {}

        constexpr Storage(Storage&& rhs) : Storage() {
            if (rhs.state_ < State::EXCEPTION_MIN) {
                state_ = rhs.state_;
            } else {
                new (&ex_) std::exception_ptr(std::move(rhs.ex_));
            }
            rhs.state_ = State::NOTHING;
        }

        ~Storage() {
            if (state_ >= State::EXCEPTION_MIN) {
                ex_.~exception_ptr();
            }
            state_ = State::NOTHING;
        }

        void set_exception(std::exception_ptr&& ex) {
            new (&ex_) std::exception_ptr(std::move(ex));
            assert(state_ >= State::EXCEPTION_MIN);
        }

        std::exception_ptr& get_exception() {
            assert(state_ >= State::EXCEPTION_MIN);
            return ex_;
        }

        constexpr const std::exception_ptr& get_exception() const {
            assert(state_ >= State::EXCEPTION_MIN);
            return ex_;
        }
    };

    constexpr TryBase() : stg_() {}
    constexpr explicit TryBase(State st) : stg_(st) {}
    explicit TryBase(std::exception_ptr&& ex) : stg_(std::move(ex)) {}

    constexpr TryBase(TryBase&& rhs) : stg_(std::move(rhs.stg_)) {}

    ~TryBase() {}

    constexpr bool has_exception() const {
        return stg_.state_ >= State::EXCEPTION_MIN;
    }

    constexpr bool has_value() const {
        return stg_.state_ == State::VALUE;
    }

    constexpr bool has_nothing() const {
        return stg_.state_ == State::NOTHING;
    }

    void set_exception(std::exception_ptr&& ex) {
        stg_.set_exception(std::move(ex));
    }

    std::exception_ptr& get_exception() {
        return stg_.get_exception();
    }

    constexpr const std::exception_ptr& get_exception() const {
        return stg_.get_exception();
    }

    constexpr void set_value() {
        stg_.state_ = State::VALUE;
    }

    constexpr void set_nothing() {
        stg_.state_ = State::NOTHING;
    }

private:
    Storage stg_;
};

template <typename T, bool Trivial>
class Uninitialized;

template <typename T>
class Uninitialized<T, false> {
public:
    constexpr Uninitialized() = default;

    template <typename... Args>
    constexpr Uninitialized(std::in_place_t, Args&&... args) {
        new (&stg_) T(std::forward<Args>(args)...);
    }

    constexpr explicit Uninitialized(T&& val) {
        new (&stg_) T(std::move(val));
    }

    constexpr explicit Uninitialized(const T& val) {
        new (&stg_) T(val);
    }

    template <typename... Args>
    constexpr T& emplace(Args&&... args) {
        new (&stg_) T(std::forward<Args>(args)...);
        return get();
    }

    constexpr void set(T val) {
        new (&stg_) T(std::move(val));
    }

    constexpr T& get() {
        return reinterpret_cast<T&>(stg_);
    }

    constexpr const T& get() const {
        return reinterpret_cast<const T&>(stg_);
    }

    constexpr void clear() {
        get().~T();
    }

private:
    std::aligned_storage_t<sizeof(T), alignof(T)> stg_;
};

template <typename T>
class Uninitialized<T, true> {
public:
    constexpr Uninitialized() = default;

    template <typename... Args>
    constexpr Uninitialized(std::in_place_t, Args&&... args)
        : value_(std::forward<Args>(args)...) {}

    constexpr explicit Uninitialized(T&& val) : value_(std::move(val)) {}

    constexpr explicit Uninitialized(const T& val) : value_(val) {}

    template <typename... Args>
    constexpr T& emplace(Args&&... args) {
        new (&value_) T(std::forward<Args>(args)...);
        return get();
    }

    constexpr void set(T val) {
        new (&value_) T(std::move(val));
    }

    constexpr T& get() {
        return value_;
    }

    constexpr const T& get() const {
        return value_;
    }

    constexpr void clear() {}

private:
    T value_;
};

template <typename T>
using UninitializedType = Uninitialized<T, std::is_trivial_v<T>>;
} // namespace detail

/// TryInvalidException
///
/// Throws when:
/// - trying to get a value from a `Try` with NOTHING state
/// - trying to get a exception from a `Try` with VALUE state
class TryInvalidException : public std::runtime_error {
public:
    TryInvalidException() : std::runtime_error("Invalid operation on Try") {}
};

/// Try
///
/// # Brief
///
/// `Try` is more likely the `Result`.
/// It can contains either a `T`, `std::exception_ptr` or nothing.
///
/// # Examples
///
/// Basic usage
///
/// ```
/// Try<std::string> concat(std::string s1, std::string s2) {
///     try {
///         s1.append(s2);
///     } catch (const std::length_error& ex) {
///         return Try<std::string>{std::current_exception()};
///     }
///     return Try<std::string>{std::move(s1)};
/// }
/// ```
template <typename T>
class Try : private detail::TryBase, private detail::UninitializedType<T> {
    static_assert(!std::is_reference_v<T>,
                  "Try cannot be used with reference types");
    static_assert(!std::is_abstract_v<T>,
                  "Try cannot be used with abstract types");

public:
    using value_type = T;

    /// Constructs nothing
    ///
    /// ```
    /// const Try<int> t();
    /// assert(t.has_nothing());
    /// ```
    constexpr Try() noexcept : detail::TryBase() {}

    /// @{
    /// Constructs a `Try` with a value by copy/move
    ///
    /// ```
    /// const Try<int> t(42);
    /// assert(t.has_value());
    /// assert(t.value() == 42);
    /// ```
    constexpr explicit Try(const T& val) noexcept(
        std::is_nothrow_copy_constructible_v<T>)
        : detail::TryBase(detail::TryBase::State::VALUE),
          detail::UninitializedType<T>(val) {}

    constexpr explicit Try(T&& val) noexcept(
        std::is_nothrow_move_constructible_v<T>)
        : detail::TryBase(detail::TryBase::State::VALUE),
          detail::UninitializedType<T>(std::move(val)) {}

    template <typename... Args>
    constexpr explicit Try(std::in_place_t, Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...>)
        : detail::TryBase(detail::TryBase::State::VALUE),
          detail::UninitializedType<T>(std::in_place,
                                       std::forward<Args>(args)...) {}
    /// @}

    /// Constructs a `Try` with an `std::exception_ptr`
    ///
    /// ```
    /// const Try<int> t(std::make_exception_ptr("exception"));
    /// assert(t.has_exception());
    /// ```
    constexpr explicit Try(std::exception_ptr&& ex) noexcept
        : detail::TryBase(std::move(ex)) {}

    /// @{
    /// Move constructor and assigner
    ///
    /// If move ctor throws exception, `Try` will be in NOTHING state
    constexpr Try(Try&& rhs) noexcept(std::is_nothrow_move_constructible_v<T>)
        : Try() {
        if (rhs.has_value()) {
            detail::UninitializedType<T>::set(
                std::move(rhs.detail::UninitializedType<T>::get()));
            detail::TryBase::set_value();
        } else if (rhs.has_exception()) {
            detail::TryBase::set_exception(
                std::move(rhs.detail::TryBase::get_exception()));
        }
    }

    constexpr Try&
    operator=(Try&& rhs) noexcept(std::is_nothrow_move_constructible_v<T>) {
        this->~Try();
        new (this) Try(std::move(rhs));
        return *this;
    }
    /// @}

    ~Try() noexcept(std::is_nothrow_destructible_v<T>) {
        if (has_value()) {
            detail::UninitializedType<T>::clear();
        }
    }

    /// Inplace constructs the value
    template <typename... Args>
    constexpr T& emplace(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...>) {
        this->~Try();
        detail::UninitializedType<T>::emplace(std::forward<Args>(args)...);
        detail::TryBase::set_value();
        return detail::UninitializedType<T>::get();
    }

    /// Inplace constructs the exception
    constexpr std::exception_ptr& emplace(std::exception_ptr&& ex) noexcept {
        this->~Try();
        detail::TryBase::set_exception(std::move(ex));
        return detail::TryBase::get_exception();
    }

    /// @{
    /// Unwraps a `Try`, yielding the value.
    /// It's similar with `value()` function except the throw conditions.
    ///
    /// Throws:
    /// - `TryInvalidException` if `Try` has nothing
    /// - EXCEPTION if `Try` has an exception
    ///
    /// # Examples
    ///
    /// ```
    /// Try<std::string> t1("hello"s);
    /// assert(t1.get() == "hello");
    ///
    /// Try<std::string> t2(std::make_exception_ptr(0xbad));
    /// std::string s = t2.get(); // ERROR: int thrown
    /// ```
    constexpr T& get() & {
        get_required();
        return detail::UninitializedType<T>::get();
    }

    constexpr const T& get() const& {
        get_required();
        return detail::UninitializedType<T>::get();
    }

    constexpr T&& get() && {
        get_required();
        return std::move(detail::UninitializedType<T>::get());
    }

    constexpr const T&& get() const&& {
        get_required();
        return std::move(detail::UninitializedType<T>::get());
    }
    /// @}

    /// @{
    /// Unwraps a `Try`, yielding the value.
    ///
    /// Throws `TryInvalidException` if `Try` doesn't have a value
    constexpr T& value() & {
        value_required();
        return detail::UninitializedType<T>::get();
    }

    constexpr const T& value() const& {
        value_required();
        return detail::UninitializedType<T>::get();
    }

    constexpr T&& value() && {
        value_required();
        return std::move(detail::UninitializedType<T>::get());
    }

    constexpr const T&& value() const&& {
        value_required();
        return std::move(detail::UninitializedType<T>::get());
    }
    /// @}
   
    /// @{
    /// Unwraps a `Try`, yielding the exception
    /// 
    /// Throws `TryInvalidException` if `Try` doesn't have an exception
    constexpr std::exception_ptr& exception() & {
        exception_required();
        return detail::TryBase::get_exception();
    }

    constexpr const std::exception_ptr& exception() const& {
        exception_required();
        return detail::TryBase::get_exception();
    }

    constexpr std::exception_ptr&& exception() && {
        exception_required();
        return std::move(detail::TryBase::get_exception());
    }

    constexpr const std::exception_ptr&& exception() const&& {
        exception_required();
        return std::move(detail::TryBase::get_exception());
    }
    /// @}

    /// @{
    /// Makes it behave like a pointer
    ///
    /// Throws `TryInvalidException` if `Try` doesn't have a value
    constexpr T* operator->() {
        return std::addressof(value());
    }

    constexpr const T* operator->() const {
        return std::addressof(value());
    }
    /// @}

    /// @{
    /// Makes it behave like a pointer
    ///
    /// Throws `TryInvalidException` if `Try` doesn't have a value
    constexpr T& operator*() & {
        return value();
    }

    constexpr const T& operator*() const& {
        return value();
    }

    constexpr T&& operator*() && {
        return std::move(value());
    }

    constexpr const T&& operator*() const&& {
        return std::move(value());
    }
    /// @}

    /// Checks whether it holds a value
    constexpr bool has_value() const noexcept {
        return detail::TryBase::has_value();
    }

    /// Checks whether it holds an exception
    constexpr bool has_exception() const noexcept {
        return detail::TryBase::has_exception();
    }

    /// Checks whether it holds nothing
    constexpr bool has_nothing() const noexcept {
        return detail::TryBase::has_nothing();
    }

private:
    constexpr void get_required() const {
        if (has_exception()) {
            std::rethrow_exception(std::move(detail::TryBase::get_exception()));
        } else if (has_nothing()) {
            throw TryInvalidException();
        }
    }

    constexpr void value_required() const {
        if (!has_value()) {
            throw TryInvalidException();
        }
    }

    constexpr void exception_required() const {
        if (!has_exception()) {
            throw TryInvalidException();
        }
    }
};
} // namespace bipolar

#endif
