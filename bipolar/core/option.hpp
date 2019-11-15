//! Option
//!
//! See `Option` type for details.
//!

#ifndef BIPOLAR_CORE_OPTION_HPP_
#define BIPOLAR_CORE_OPTION_HPP_

#include <cassert>
#include <cstdint>
#include <functional>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "bipolar/core/internal/enable_special_members.hpp"

namespace bipolar {
// forward
template <typename>
class Option;

namespace detail {
// None variant
struct None {
    enum class Secret {
        TOKEN,
    };

    constexpr explicit None(Secret) noexcept {}
};

// Option traits
template <typename T>
struct is_option_impl : std::false_type {};

template <typename T>
struct is_option_impl<Option<T>> : std::true_type {};

template <typename T>
using is_option = is_option_impl<std::decay_t<T>>;

template <typename T>
inline constexpr bool is_option_v = is_option<T>::value;

// To support constexpr for Option<T>
template <typename T>
union OptionTrivialStorage {
    T value;
    char dummy;

    constexpr OptionTrivialStorage() : dummy('\0') {}

    template <typename... Args>
    constexpr OptionTrivialStorage(Args&&... args)
        : value(std::forward<Args>(args)...) {}

    ~OptionTrivialStorage() = default;
};

template <typename T>
union OptionNonTrivialStorage {
    T value;
    char dummy;

    constexpr OptionNonTrivialStorage() : dummy('\0') {}

    template <typename... Args>
    constexpr OptionNonTrivialStorage(Args&&... args)
        : value(std::forward<Args>(args)...) {}

    ~OptionNonTrivialStorage() {}
};

template <typename T, bool = std::is_trivially_destructible_v<T>>
class OptionBase;

template <typename T>
class OptionBase<T, true> {
public:
    constexpr OptionBase() : has_value_(false) {}

    constexpr explicit OptionBase(T&& val)
        : has_value_(true), sto_(std::move(val)) {}

    constexpr explicit OptionBase(const T& val) : has_value_(true), sto_(val) {}

    template <typename... Args>
    constexpr OptionBase(std::in_place_t, Args&&... args)
        : has_value_(true), sto_(std::forward<Args>(args)...) {}

    ~OptionBase() = default;

    constexpr void clear() {
        has_value_ = false;
    }

protected:
    bool has_value_;
    OptionTrivialStorage<T> sto_;
};

template <typename T>
class OptionBase<T, false> {
public:
    constexpr OptionBase() : has_value_(false) {}

    constexpr explicit OptionBase(T&& val)
        : has_value_(true), sto_(std::move(val)) {}

    constexpr explicit OptionBase(const T& val) : has_value_(true), sto_(val) {}

    template <typename... Args>
    constexpr OptionBase(std::in_place_t, Args&&... args)
        : has_value_(true), sto_(std::forward<Args>(args)...) {}

    ~OptionBase() {
        clear();
    }

    constexpr void clear() {
        if (has_value_) {
            has_value_ = false;
            sto_.value.~T();
        }
    }

protected:
    bool has_value_;
    OptionNonTrivialStorage<T> sto_;
};
} // namespace detail

/// OptionEmptyException
///
/// Throws when trying to unwrap an empty `Option`
class OptionEmptyException : public std::runtime_error {
public:
    OptionEmptyException()
        : std::runtime_error("Empty Option cannot be unwrapped") {}

    explicit OptionEmptyException(const char* s) : std::runtime_error(s) {}
};

/// None variant of `Option`
inline constexpr detail::None None{detail::None::Secret::TOKEN};

/// `Some` variant of `Option`
///
/// See `Option::Option` for details
template <typename T>
inline constexpr Option<T>
Some(T&& val) noexcept(std::is_nothrow_move_constructible_v<T>) {
    return {std::move(val)};
}

template <typename T>
inline constexpr Option<T>
Some(const T& val) noexcept(std::is_nothrow_copy_constructible_v<T>) {
    return {val};
}

/// Option
///
/// # Brief
///
/// `Option` type represents an optional value. Either one is `Some` and
/// contains a value, or `None`, and does not.
///
/// It has many usages:
/// - Init values
/// - Return value for otherwise reporting simple errors, where `None` is
///   returned on error
/// - Optional struct fields
/// - Optional function arguments
///
/// # Examples
///
/// Basic usage
///
/// ```
/// Option<double> divide(int numerator, int denominator) {
///     if (denominator == 0) {
///         return None;
///     } else {
///         return Some(1.0 * numerator / denominator);
///     }
/// }
///
/// const auto result = divide(1, 0);
/// assert(!result.has_value());
/// ```
///
/// Indicates an error instead of using out parameter and bool return value
///
/// ```
/// Option<std::size_t> find(const std::vector<int>& vec, int target) {
///     for (std::size_t i = 0; i < vec.size(); ++i) {
///         if (vec[i] == target) {
///             return Some(i);
///         }
///     }
///     return None;
/// }
/// ```
///
template <typename T>
class Option : public detail::OptionBase<T>,
               public internal::EnableCopyConstructor<
                   std::is_copy_constructible_v<T>,
                   std::is_nothrow_copy_constructible_v<T>, Option<T>>,
               public internal::EnableCopyAssignment<
                   std::conjunction_v<std::is_copy_constructible<T>,
                                      std::is_copy_assignable<T>>,
                   std::conjunction_v<std::is_nothrow_copy_constructible<T>,
                                      std::is_nothrow_copy_assignable<T>>,
                   Option<T>>,
               public internal::EnableMoveConstructor<
                   std::is_move_constructible_v<T>,
                   std::is_nothrow_move_constructible_v<T>, Option<T>>,
               public internal::EnableMoveAssignment<
                   std::conjunction_v<std::is_move_constructible<T>,
                                      std::is_move_assignable<T>>,
                   std::conjunction_v<std::is_nothrow_move_constructible<T>,
                                      std::is_nothrow_move_assignable<T>>,
                   Option<T>> {

    static_assert(!std::is_reference_v<T>,
                  "Option cannot be used with reference types");
    static_assert(!std::is_abstract_v<T>,
                  "Option cannot be used with abstract types");

    using Base = detail::OptionBase<T>;

public:
    using value_type = T;

    /// No value
    ///
    /// ```
    /// const Option<int> none(None);
    /// assert(!none.has_value());
    /// ```
    constexpr Option() noexcept : Base() {}
    constexpr /*implicit*/ Option(detail::None) noexcept : Base() {}

    /// Constructs from value directly
    ///
    /// ```
    /// const Option<int> opt(42);
    /// assert(opt.has_value());
    /// ```
    constexpr /*implicit*/ Option(T&& val) noexcept(
        std::is_nothrow_move_constructible_v<T>)
        : Base(std::move(val)) {}

    constexpr /*implicit*/ Option(const T& val) noexcept(
        std::is_nothrow_copy_constructible_v<T>)
        : Base(val) {}

    /// Constructs from other options
    constexpr Option(Option&& rhs) noexcept(
        std::is_nothrow_move_constructible_v<T>)
        : Base() {
        if (rhs.has_value()) {
            construct(std::move(rhs.Base::sto_.value));
            rhs.clear();
        }
    }

    constexpr Option(const Option& rhs) noexcept(
        std::is_nothrow_copy_constructible_v<T>)
        : Base() {
        if (rhs.has_value()) {
            construct(rhs.Base::sto_.value);
        }
    }

    /// Resets to `None`
    ///
    /// ```
    /// Option<int> res(Some(42));
    /// assert(res.has_value());
    ///
    /// res.assign(None);
    /// assert(!res.has_value());
    /// ```
    constexpr void assign(detail::None) noexcept {
        clear();
    }

    /// Assigns other options
    constexpr void
    assign(Option&& rhs) noexcept(std::is_nothrow_move_constructible_v<T>) {
        if (this != std::addressof(rhs)) {
            if (rhs.has_value()) {
                assign(std::move(rhs.Base::sto_.value));
                rhs.clear();
            } else {
                clear();
            }
        }
    }

    constexpr void assign(const Option& rhs) noexcept(
        std::is_nothrow_copy_constructible_v<T>) {
        if (rhs.has_value()) {
            assign(rhs.Base::sto_.value);
        } else {
            clear();
        }
    }

    /// Assigns value directly
    ///
    /// ```
    /// Option<int> opt(None);
    /// assert(!opt.has_value());
    ///
    /// opt.assign(10);
    /// assert(opt.has_value());
    /// assert(opt.value() == 10);
    /// ```
    constexpr void
    assign(T&& val) noexcept(std::is_nothrow_move_constructible_v<T>) {
        if (has_value()) {
            Base::sto_.value = std::move(val);
        } else {
            construct(std::move(val));
        }
    }

    constexpr void
    assign(const T& val) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        if (has_value()) {
            Base::sto_.value = val;
        } else {
            construct(val);
        }
    }

    /// Resets to `None`
    constexpr Option& operator=(detail::None) noexcept {
        clear();
        return *this;
    }

    /// Assigns other options
    constexpr Option&
    operator=(Option&& rhs) noexcept(std::is_nothrow_move_constructible_v<T>) {
        assign(std::move(rhs));
        return *this;
    }

    constexpr Option& operator=(const Option& rhs) noexcept(
        std::is_nothrow_copy_constructible_v<T>) {
        assign(rhs);
        return *this;
    }

    /// Placement constructs from arguments
    ///
    /// ```
    /// Option<std::string> sopt(None);
    ///
    /// sopt.emplace("hello");
    /// assert(sopt.has_value());
    /// assert(sopt.value() == "hello");
    /// ```
    template <typename... Args>
    constexpr T&
    emplace(Args&&... args) noexcept(std::is_nothrow_move_constructible_v<T>) {
        clear();
        construct(std::forward<Args>(args)...);
        return value();
    }

    /// Resets to None
    ///
    /// ```
    /// Option<int> x(Some(1));
    /// assert(x.has_value());
    ///
    /// x.clear();
    /// assert(!x.has_value());
    /// ```
    constexpr void clear() noexcept {
        Base::clear();
    }

    /// Swaps with other options
    ///
    /// ```
    /// Option<int> x(Some(1));
    /// assert(x.has_value() && x.value() == 1);
    ///
    /// Option<int> none(None);
    /// assert(!none.has_value());
    ///
    /// x.swap(none);
    /// assert(!x.has_value());
    /// assert(none.has_value() && none.value() == 1);
    /// ```
    constexpr void swap(Option& rhs) noexcept(std::is_nothrow_swappable_v<T>) {
        if (has_value() && rhs.has_value()) {
            using std::swap;
            swap(value(), rhs.value());
        } else if (has_value()) {
            rhs.assign(std::move(value()));
            clear();
        } else if (rhs.has_value()) {
            assign(std::move(rhs.value()));
            rhs.clear();
        }
    }

    /// Unwraps an option, yielding the content of a `Some`
    ///
    /// throws `OptionEmptyException` if option is `None`
    constexpr T& value() & {
        value_required();
        return Base::sto_.value;
    }

    constexpr const T& value() const& {
        value_required();
        return Base::sto_.value;
    }

    constexpr T&& value() && {
        value_required();
        return std::move(Base::sto_.value);
    }

    constexpr const T&& value() const&& {
        value_required();
        return std::move(Base::sto_.value);
    }

    /// Unwraps an option, yielding the content of a `Some`
    ///
    /// throws `OptionEmptyException` with custom message if option is `None`
    constexpr const T& expect(const char* s) const& {
        value_required(s);
        return Base::sto_.value;
    }

    constexpr T&& expect(const char* s) && {
        value_required(s);
        return std::move(Base::sto_.value);
    }

    /// Returns the contained value or a default
    ///
    /// Arguments passed to `value_or` are eagerly evaluated; if you are
    /// passing the result of a function call, it is recommended to use
    /// `value_or_else`, which is lazily evaluated
    ///
    /// # Constraints
    ///
    /// `U` can be convertible to `T`
    template <typename U>
    constexpr T value_or(U&& deft) const& {
        return has_value() ? value() : static_cast<T>(std::forward<U>(deft));
    }

    template <typename U>
    constexpr T value_or(U&& deft) && {
        return has_value() ? std::move(value())
                           : static_cast<T>(std::forward<U>(deft));
    }
    /// @}

    /// Returns the contained value or the result of function
    ///
    /// # Constraints
    ///
    /// F :: () -> T
    ///
    /// # Examples
    ///
    /// Basic usage
    ///
    /// ```
    /// const int k = 10;
    /// const Option<int> none(None);
    /// const Option<int> opt(Some(1));
    ///
    /// assert(none.value_or_else([&]() { return 2 * k; }) == 20);
    /// assert(opt.value_or_else([&]() { return 2 * k; }) == 1);
    /// ```
    template <
        typename F,
        std::enable_if_t<std::is_same_v<std::invoke_result_t<F>, T>, int> = 0>
    constexpr T value_or_else(F&& f) const& {
        return has_value() ? value() : std::forward<F>(f)();
    }

    template <
        typename F,
        std::enable_if_t<std::is_same_v<std::invoke_result_t<F>, T>, int> = 0>
    constexpr T value_or_else(F&& f) && {
        return has_value() ? std::move(value()) : std::forward<F>(f)();
    }

    /// Maps an `Option<T>` to `Option<U>` by applying a function to
    /// the contained value
    ///
    /// # Constraints
    ///
    /// F :: T -> U
    ///
    /// # Examples
    ///
    /// ```
    /// const auto opt = Some(1);
    /// const auto res = opt.map([](int x) -> std::string {
    ///     char buf[32];
    ///     std::snprintf(buf, sizeof(buf), "%d", x);
    ///     return std::string(buf);
    /// });
    ///
    /// assert(res.has_value() && res.value() == "1");
    /// ```
    template <typename F, typename U = std::invoke_result_t<F, T>>
    constexpr Option<U> map(F&& f) const& {
        return has_value() ? Some(std::forward<F>(f)(value())) : None;
    }

    template <typename F, typename U = std::invoke_result_t<F, T>>
    constexpr Option<U> map(F&& f) && {
        return has_value() ? Some(std::forward<F>(f)(std::move(value())))
                           : None;
    }

    /// Applies the function to the contained value or returns
    /// the provided default value
    ///
    /// # Constraints
    ///
    /// F :: T -> U
    ///
    /// # Examples
    ///
    /// Basic usage
    ///
    /// ```
    /// const auto opt = Some(0);
    /// const auto res = opt.map_or(Some(-1),
    ///                             [](int x) -> int { return x + 1; });
    /// assert(res.has_value() && res.value() == 1);
    ///
    /// const Option<int> none(None);
    /// const auto res2 = none.map_or(Some(-1),
    ///                               [](int x) -> int { return x + 1; });
    /// assert(res2.has_value() && res2.value() == -1);
    /// ```
    template <typename F, typename U = std::invoke_result_t<F, T>>
    constexpr U map_or(U deft, F&& f) const& {
        return has_value() ? std::forward<F>(f)(value()) : std::move(deft);
    }

    template <typename F, typename U = std::invoke_result_t<F(T)>>
    constexpr U map_or(U deft, F&& f) && {
        return has_value() ? std::forward<F>(f)(std::move(value()))
                           : std::move(deft);
    }

    /// Applies the function to the contained value or return the result
    /// of functor `d`
    ///
    /// # Constraints
    ///
    /// F :: T -> U
    /// D :: () -> U
    ///
    /// # Examples
    ///
    /// Basic usage
    ///
    /// ```
    /// const auto opt = Some(0);
    /// const auto res =
    ///     opt.map_or_else([]() -> Option<int> { return Some(-1); },
    ///                     [](int x) -> int { return x + 1; });
    /// assert(res.has_value() && res.value() == 1);
    ///
    /// const Option<int> none(None);
    /// const auto res2 =
    ///     none.map_or_else([]() -> Option<int> { return Some(-1); },
    ///                      [](int x) -> int { return x + 1; });
    /// assert(res2.has_value() && res2.value() == -1);
    /// ```
    template <
        typename F, typename D, typename U = std::invoke_result_t<F, T>,
        std::enable_if_t<std::is_same_v<U, std::invoke_result_t<D>>, int> = 0>
    constexpr U map_or_else(D&& d, F&& f) const& {
        return has_value() ? std::forward<F>(f)(value()) : std::forward<D>(d)();
    }

    template <
        typename F, typename D, typename U = std::invoke_result_t<F, T>,
        std::enable_if_t<std::is_same_v<U, std::invoke_result_t<D>>, int> = 0>
    constexpr U map_or_else(D&& d, F&& f) && {
        return has_value() ? std::forward<F>(f)(std::move(value()))
                           : std::forward<D>(d)();
    }

    /// Returns `None` if the option is `None`, otherwise calls `f` with
    /// the wrapped value and returns the result
    ///
    /// # Constraints
    ///
    /// F :: const T& -> Option<U>
    ///
    /// # Examples
    ///
    /// Basic usage
    ///
    /// ```
    /// Option<int> square(int x) { return Some(x * x); }
    /// Option<int> add1(int x) { return Some(x + 1); }
    ///
    /// const Option<int> none(None);
    /// const Option<int> x(Some(3));
    ///
    /// const auto res1 = none.and_then(square).and_then(add1);
    /// assert(!res1.has_value());
    ///
    /// const auto res2 = x.and_then(add1), and_then(square);
    /// assert(res2.has_value() && res2.value() == 16);
    /// ```
    template <typename F,
              typename Arg = std::add_lvalue_reference_t<std::add_const_t<T>>,
              typename R = std::invoke_result_t<F, Arg>,
              std::enable_if_t<detail::is_option_v<R>, int> = 0>
    constexpr R and_then(F&& f) const& {
        return has_value() ? std::forward<F>(f)(value()) : None;
    }

    /// Returns `None` if the option is `None`, otherwise
    /// calls `f` with the wrapped value and returns the result
    ///
    /// # Constraints
    ///
    /// F :: T -> Option<U>
    template <typename F, typename R = std::invoke_result_t<F, T>,
              std::enable_if_t<detail::is_option_v<R>, int> = 0>
    constexpr R and_then(F&& f) && {
        return has_value() ? std::forward<F>(f)(std::move(value())) : None;
    }

    /// Returns `None` if the option is `None`, otherwise calls `f`
    /// with the wrapped value and returns:
    /// - `Some(t)` if `f` returns `true` where `t` is the wrapped value, and
    /// - `None` if `f` returns `false`
    ///
    /// # Constraints
    ///
    /// F :: const T& -> bool
    ///
    /// # Examples
    ///
    /// Basic usage
    ///
    /// ```
    /// const auto x = Some(13);
    /// const auto res = x.filter([](int x) { return x % 2 == 0; });
    /// assert(!res.has_value());
    /// ```
    template <typename F,
              typename Arg = std::add_lvalue_reference_t<std::add_const_t<T>>,
              std::enable_if_t<
                  std::is_same_v<std::invoke_result_t<F, Arg>, bool>, int> = 0>
    constexpr Option<T> filter(F&& f) const& {
        return has_value() && std::forward<F>(f)(value()) ? *this : None;
    }

    template <typename F,
              typename Arg = std::add_lvalue_reference_t<std::add_const_t<T>>,
              std::enable_if_t<
                  std::is_same_v<std::invoke_result_t<F, Arg>, bool>, int> = 0>
    constexpr Option<T> filter(F&& f) && {
        return has_value() && std::forward<F>(f)(value()) ? std::move(*this)
                                                          : None;
    }

    /// Returns the option if it contains a value, otherwise
    /// calls `f` and returns the result
    ///
    /// # Constaints
    ///
    /// F :: () -> Option<T>
    ///
    /// # Examples
    ///
    /// Basic usage
    ///
    /// ```
    /// const auto opt = Some(0);
    /// const Result<int> none(None);
    ///
    /// const auto res = none.or_else([=]() { return opt; });
    /// assert(res.has_value() && res.value() == 0);
    ///
    /// const auto res2 = opt.or_else([]() -> Option<int> {
    ///     return Some(10);
    /// });
    /// assert(res2.has_value() && res2.value() == 0);
    /// ```
    template <typename F, typename U = std::invoke_result_t<F>,
              std::enable_if_t<std::is_same_v<U, Option<T>>, int> = 0>
    constexpr Option<T> or_else(F&& f) const& {
        return has_value() ? *this : std::forward<F>(f)();
    }

    template <typename F, typename U = std::invoke_result_t<F>,
              std::enable_if_t<std::is_same_v<U, Option<T>>, int> = 0>
    constexpr Option<T> or_else(F&& f) && {
        return has_value() ? std::move(*this) : std::forward<F>(f)();
    }

    /// Takes the value out of the option, leaving a `None` in place
    ///
    /// ```
    /// Option<std::string> opt("hello");
    ///
    /// auto out = opt.take();
    /// assert(out.value() == "hello");
    /// assert(!opt.has_value());
    /// ```
    constexpr Option<T> take() & {
        Option<T> opt(std::move(*this));
        return opt;
    }

    /// Returns the pointer to internal storage
    ///
    /// If the value is a `None`, `nullptr` is returned
    constexpr T* get_pointer() & {
        return has_value() ? std::addressof(Base::sto_.value) : nullptr;
    }

    constexpr const T* get_pointer() const& {
        return has_value() ? std::addressof(Base::sto_.value) : nullptr;
    }

    T* get_pointer() && = delete;

    /// Checks whether the option is `Some` or not
    constexpr bool has_value() const noexcept {
        return Base::has_value_;
    }

    /// Checks whether the option is `Some` or not
    constexpr operator bool() const noexcept {
        return has_value();
    }

    /// Dereference makes it more like a pointer
    /// Throws `OptionEmptyException` if the option is `None`
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

    /// Arrow operator makes it more like a pointer
    /// Throws `OptionEmptyException` if the option is `None`
    constexpr const T* operator->() const {
        return &value();
    }

    constexpr T* operator->() {
        return &value();
    }

private:
    constexpr void value_required() const {
        if (!Base::has_value_) {
            throw OptionEmptyException();
        }
    }

    constexpr void value_required(const char* s) const {
        if (!Base::has_value_) {
            throw OptionEmptyException(s);
        }
    }

    template <typename... Args>
    constexpr void construct(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...>) {
        new (const_cast<T*>(std::addressof(Base::sto_.value)))
            T(std::forward<Args>(args)...);
        Base::has_value_ = true;
    }
};

/// Swaps with other options
template <typename T>
inline constexpr void swap(Option<T>& lhs,
                           Option<T>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

/// Comparison operators for `Option<T>`s
template <typename T>
inline constexpr bool operator==(const Option<T>& lhs, const Option<T>& rhs) {
    if (lhs.has_value() != rhs.has_value()) {
        return false;
    }
    if (lhs.has_value()) {
        return lhs.value() == rhs.value();
    }
    return true;
}

template <typename T>
inline constexpr bool operator!=(const Option<T>& lhs, const Option<T>& rhs) {
    return !(lhs == rhs);
}

template <typename T>
inline constexpr bool operator<(const Option<T>& lhs, const Option<T>& rhs) {
    if (lhs.has_value() != rhs.has_value()) {
        return static_cast<int>(lhs.has_value()) <
               static_cast<int>(rhs.has_value());
    }
    if (lhs.has_value()) {
        return lhs.value() < rhs.value();
    }
    return false;
}

template <typename T>
inline constexpr bool operator>(const Option<T>& lhs, const Option<T>& rhs) {
    return rhs < lhs;
}

template <typename T>
inline constexpr bool operator<=(const Option<T>& lhs, const Option<T>& rhs) {
    return !(lhs > rhs);
}

template <typename T>
inline constexpr bool operator>=(const Option<T>& lhs, const Option<T>& rhs) {
    return !(lhs < rhs);
}

/// Comparison operators for `Option<T>` and `None`, where
/// `None` is in the right hand
template <typename T>
inline constexpr bool operator==(const Option<T>& lhs, detail::None) noexcept {
    return !lhs.has_value();
}

template <typename T>
inline constexpr bool operator!=(const Option<T>& lhs, detail::None) noexcept {
    return lhs.has_value();
}

template <typename T>
inline constexpr bool operator<(const Option<T>& lhs, detail::None) noexcept {
    (void)lhs;
    return false;
}

template <typename T>
inline constexpr bool operator>(const Option<T>& lhs, detail::None) noexcept {
    return lhs.has_value();
}

template <typename T>
inline constexpr bool operator<=(const Option<T>& lhs, detail::None) noexcept {
    return !lhs.has_value();
}

template <typename T>
inline constexpr bool operator>=(const Option<T>& lhs, detail::None) noexcept {
    (void)lhs;
    return true;
}

/// Comparison operators for `Option<T>` and `None`, where
/// `None` is in the left hand
template <typename T>
inline constexpr bool operator==(detail::None, const Option<T>& rhs) noexcept {
    return !rhs.has_value();
}

template <typename T>
inline constexpr bool operator!=(detail::None, const Option<T>& rhs) noexcept {
    return rhs.has_value();
}

template <typename T>
inline constexpr bool operator<(detail::None, const Option<T>& rhs) noexcept {
    return rhs.has_value();
}

template <typename T>
inline constexpr bool operator>(detail::None, const Option<T>& rhs) noexcept {
    (void)rhs;
    return false;
}

template <typename T>
inline constexpr bool operator<=(detail::None, const Option<T>& rhs) noexcept {
    (void)rhs;
    return true;
}

template <typename T>
inline constexpr bool operator>=(detail::None, const Option<T>& rhs) noexcept {
    return !rhs.has_value();
}

} // namespace bipolar

namespace std {
template <typename T>
struct hash<bipolar::Option<T>> {
    std::size_t operator()(const bipolar::Option<T>& opt) const {
        if (!opt.has_value()) {
            return 0;
        }
        return hash<remove_const_t<T>>()(opt.value());
    }
};
} // namespace std

#endif
