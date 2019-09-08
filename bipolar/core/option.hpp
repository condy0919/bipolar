#ifndef BIPOLAR_CORE_OPTION_HPP_
#define BIPOLAR_CORE_OPTION_HPP_

/// \file option.hpp


#include <new>
#include <cassert>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <utility>


namespace bipolar {
template <typename>
class Option;

namespace detail {
struct None {
    enum class _secret { _token };

    explicit constexpr None(_secret) noexcept {}
};

template <typename T>
struct is_option_impl : std::false_type {};

template <typename T>
struct is_option_impl<Option<T>> : std::true_type {};

template <typename T>
using is_option = is_option_impl<std::decay_t<T>>;

template <typename T>
inline constexpr bool is_option_v = is_option<T>::value;
} // namespace detail


/// \class OptionEmptyException
/// \brief Throws when trying to unwrap an empty Option
class OptionEmptyException : public std::runtime_error {
public:
    OptionEmptyException()
        : std::runtime_error("Empty Option cannot be unwrapped") {}

    OptionEmptyException(const char* s) : std::runtime_error(s) {}
};

/// \brief No value
constexpr detail::None None{detail::None::_secret::_token};

/// @{
/// \brief Some value `T`
/// \return Option\<T\>
/// \see Option::Option
template <typename T>
constexpr Option<T> Some(T&& val) noexcept(
    std::is_nothrow_move_constructible_v<T>) {
    return {std::move(val)};
}

template <typename T>
constexpr Option<T> Some(const T& val) noexcept(
    std::is_nothrow_copy_constructible_v<T>) {
    return {val};
}
/// @}

/// \class Option
/// \brief Type Option represents an optional value. Either one is `Some` and
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
/// ```cpp
/// Option<double> divide(int numerator, int denominator) {
///     if (denominator == 0) {
///         return None;
///     } else {
///         return Some(1.0 * numerator / denominator);
///     }
/// }
///
/// const auto result = divide(1, 0);
/// assert(result.has_value());
/// ```
///
/// Indicate an error instead of using out parameter and bool return value
///
/// ```cpp
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
class Option {
    static_assert(!std::is_reference_v<T>,
                  "Option cannot be used with reference types");
    static_assert(!std::is_abstract_v<T>,
                  "Option cannot be used with abstract types");

public:
    using value_type = T;

    /// @{
    /// \brief No value
    ///
    /// ```cpp
    /// const Option<int> none(None);
    /// assert(!none.has_value());
    /// ```
    constexpr Option() noexcept {}

    constexpr Option(detail::None) noexcept {}
    /// @}

    /// @{
    /// \brief Constructs from value directly
    ///
    /// ```cpp
    /// const Option<int> opt(42);
    /// assert(opt.has_value());
    /// ```
    constexpr Option(T&& val) noexcept(
        std::is_nothrow_move_constructible_v<T>) {
        construct(std::move(val));
    }

    constexpr Option(const T& val) noexcept(
        std::is_nothrow_copy_constructible_v<T>) {
        construct(val);
    }
    /// @}

    /// @{
    /// \brief Constructs from others
    Option(Option&& rhs) noexcept(std::is_nothrow_move_constructible_v<T>) {
        if (rhs.has_value()) {
            construct(std::move(rhs.value()));
            rhs.clear();
        }
    }

    Option(const Option& rhs) noexcept(
        std::is_nothrow_copy_constructible_v<T>) {
        if (rhs.has_value()) {
            construct(rhs.value());
        }
    }
    /// @}

    /// \brief Assigns with None will reset the option
    ///
    /// ```cpp
    /// Option<int> res(Some(42));
    /// assert(res.has_value());
    ///
    /// res.assign(None);
    /// assert(!res.has_value());
    /// ```
    void assign(detail::None) {
        clear();
    }

    /// @{
    /// \brief Assigns with another Option value
    void assign(Option&& rhs) noexcept(
        std::is_nothrow_move_constructible_v<T>) {
        if (this != std::addressof(rhs)) {
            if (rhs.has_value()) {
                assign(std::move(rhs.value()));
                rhs.clear();
            } else {
                clear();
            }
        }
    }

    void assign(const Option& rhs) noexcept(
        std::is_nothrow_copy_constructible_v<T>) {
        if (rhs.has_value()) {
            assign(rhs.value());
        } else {
            clear();
        }
    }
    /// @}

    /// @{
    /// \brief Assigns with value directly
    ///
    /// ```cpp
    /// Option<int> opt(None);
    /// assert(!opt.has_value());
    ///
    /// opt.assign(10);
    /// assert(opt.has_value());
    /// assert(opt.value() == 10);
    /// ```
    void assign(T&& val) noexcept(std::is_nothrow_move_constructible_v<T>) {
        if (has_value()) {
            storage_.value = std::move(val);
        } else {
            construct(std::move(val));
        }
    }

    void assign(const T& val) noexcept(
        std::is_nothrow_copy_constructible_v<T>) {
        if (has_value()) {
            storage_.value = val;
        } else {
            construct(val);
        }
    }
    /// @}

    /// \brief Same with `assign(detail::None)`
    /// \see assign(detail::None)
    Option& operator=(detail::None) noexcept {
        clear();
        return *this;
    }

    /// @{
    /// \brief Same with `assign(Option&&)`
    /// \see assign(Option&&)
    Option& operator=(Option&& rhs) noexcept(
        std::is_nothrow_move_constructible_v<T>) {
        assign(std::move(rhs));
        return *this;
    }

    /// \brief Same with `assign(const Option&)`
    /// \see assign(const Option&)
    Option& operator=(const Option& rhs) noexcept(
        std::is_nothrow_copy_constructible_v<T>) {
        assign(rhs);
        return *this;
    }
    /// @}


    /// \brief Inplacement constructs from args
    /// \return T&
    ///
    /// ```cpp
    /// Option<std::string> sopt(None);
    ///
    /// sopt.emplace("hello");
    /// assert(sopt.has_value());
    /// assert(sopt.value() == "hello");
    /// ```
    template <typename... Args>
    T& emplace(Args&&... args) noexcept(
        std::is_nothrow_move_constructible_v<T>) {
        clear();
        construct(std::forward<Args>(args)...);
        return value();
    }

    /// \brief Resets to None
    ///
    /// ```cpp
    /// Option<int> x(Some(1));
    /// assert(x.has_value());
    ///
    /// x.clear();
    /// assert(!x.has_value());
    /// ```
    void clear() noexcept {
        storage_.clear();
    }

    /// \brief Swaps with other option
    ///
    /// ```cpp
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
    void swap(Option& rhs) noexcept(std::is_nothrow_swappable_v<T>) {
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

    /// @{
    /// \brief Unwraps an option, yeilding the content of a `Some`
    /// \throw OptionEmptyException
    constexpr T& value() & {
        value_required();
        return storage_.value;
    }

    constexpr const T& value() const& {
        value_required();
        return storage_.value;
    }

    constexpr T&& value() && {
        value_required();
        return std::move(storage_.value);
    }

    constexpr const T&& value() const&& {
        value_required();
        return std::move(storage_.value);
    }
    /// @}

    /// @{
    /// \brief Unwraps an option, yeilding the content of a `Some`
    /// \throw OptionEmptyException with custom message
    constexpr const T& expect(const char* s) const& {
        value_required(s);
        return storage_.value;
    }

    constexpr T&& expect(const char* s) && {
        value_required(s);
        return std::move(storage_.value);
    }
    /// @}

    /// @{
    /// \brief Returns the contained value or a default
    /// \return T
    ///
    /// Arguments passed to `value_or` are eagerly evaluated; if you are
    /// passing the result of a function call, it is recommended to use
    /// `value_or_else`, which is lazily evaluated
    /// \see value_or_else
    template <typename U>
    constexpr T value_or(U&& deft) const& {
        return has_value() ? value()
                           : static_cast<T>(std::forward<U>(deft));
    }

    template <typename U>
    constexpr T value_or(U&& deft) && {
        return has_value() ? std::move(value())
                           : static_cast<T>(std::forward<U>(deft));
    }
    /// @}

    /// @{
    /// \brief Returns the contained value or the result of function
    /// \return T
    ///
    /// ```cpp
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
    /// @}

    /// @{
    /// \brief Maps an `Option<T>` to `Option<U>` by applying a function to
    /// the contained value
    ///
    /// \tparam F :: T -> U
    /// \return Option\<U\>
    ///
    /// ```cpp
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
    Option<U> map(F&& f) const& {
        return has_value() ? Some(std::forward<F>(f)(value()))
                           : None;
    }

    template <typename F, typename U = std::invoke_result_t<F, T>>
    Option<U> map(F&& f) && {
        return has_value() ? Some(std::forward<F>(f)(std::move(value())))
                           : None;
    }
    /// @}

    /// @{
    /// \brief Applies the function to the contained value or returns
    /// the provided default value
    ///
    /// \tparam F :: T -> U
    /// \return U
    ///
    /// ```cpp
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
    U map_or(U deft, F&& f) const& {
        return has_value() ? std::forward<F>(f)(value())
                           : std::move(deft);
    }

    template <typename F, typename U = std::invoke_result_t<F(T)>>
    U map_or(U deft, F&& f) && {
        return has_value() ? std::forward<F>(f)(std::move(value()))
                           : std::move(deft);
    }
    /// @}

    /// @{
    /// \brief Applies the function to the contained value or
    /// return the result of functor \c d
    ///
    /// \tparam F :: T -> U
    /// \tparam D :: () -> U
    /// \return U
    ///
    /// ```cpp
    /// const auto opt = Some(0);
    /// const auto res = opt.map_or_else([]() -> Option<int> { return Some(-1); },
    ///                                  [](int x) -> int { return x + 1; });
    /// assert(res.has_value() && res.value() == 1);
    ///
    /// const Option<int> none(None);
    /// const auto res2 = none.map_or_else([]() -> Option<int> { return Some(-1); },
    ///                                    [](int x) -> int { return x + 1; });
    /// assert(res2.has_value() && res2.value() == -1);
    /// ```
    template <
        typename F, typename D, typename U = std::invoke_result_t<F, T>,
        std::enable_if_t<std::is_same_v<U, std::invoke_result_t<D>>, int> = 0>
    U map_or_else(D&& d, F&& f) const& {
        return has_value() ? std::forward<F>(f)(value())
                           : std::forward<D>(d)();
    }

    template <
        typename F, typename D, typename U = std::invoke_result_t<F, T>,
        std::enable_if_t<std::is_same_v<U, std::invoke_result_t<D>>, int> = 0>
    U map_or_else(D&& d, F&& f) && {
        return has_value() ? std::forward<F>(f)(std::move(value()))
                           : std::forward<D>(d)();
    }
    /// @}

    /// @{
    /// \brief Returns `None` if the option is `None`, otherwise calls `f` with
    /// the wrapped value and returns the result
    ///
    /// \tparam F :: const T& -> Option\<U\>
    /// \return Option\<U\>
    ///
    /// ```cpp
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
    R and_then(F&& f) const& {
        return has_value() ? std::forward<F>(f)(value())
                           : None;
    }

    /// \brief Returns `None` if the option is `None`, otherwise
    /// calls \c f with the wrapped value and returns the result
    ///
    /// \tparam F :: T -> Option\<U\>
    /// \return Option\<U\>
    template <typename F, typename R = std::invoke_result_t<F, T>,
              std::enable_if_t<detail::is_option_v<R>, int> = 0>
    R and_then(F&& f) && {
        return has_value() ? std::forward<F>(f)(std::move(value()))
                           : None;
    }
    /// @}

    /// @{
    /// \brief Returns `None` if the option is `None`, otherwise calls `f`
    /// with the wrapped value and returns:
    /// - `Some(t)` if `f` returns `true` where `t` is the wrapped value, and
    /// - `None` if `f` returns `false`
    ///
    /// \tparam F :: const T& -> bool
    /// \return Option\<T\>
    ///
    /// ```cpp
    /// const auto x = Some(13);
    /// const auto res = x.filter([](int x) { return x % 2 == 0; });
    /// assert(!res.has_value());
    /// ```
    template <typename F,
              typename Arg = std::add_lvalue_reference_t<std::add_const_t<T>>,
              std::enable_if_t<
                  std::is_same_v<std::invoke_result_t<F, Arg>, bool>, int> = 0>
    Option<T> filter(F&& f) const& {
        return has_value() && std::forward<F>(f)(value()) ? *this
                                                          : None;
    }

    template <typename F,
              typename Arg = std::add_lvalue_reference_t<std::add_const_t<T>>,
              std::enable_if_t<
                  std::is_same_v<std::invoke_result_t<F, Arg>, bool>, int> = 0>
    Option<T> filter(F&& f) && {
        return has_value() && std::forward<F>(f)(value()) ? std::move(*this)
                                                          : None;
    }
    /// @}

    /// @{
    /// \brief Returns the option if it contains a value, otherwise
    /// calls \c f and returns the result
    ///
    /// \tparam F :: () -> Option\<T\>
    /// \return Option\<T\>
    ///
    /// ```cpp
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
    Option<T> or_else(F&& f) const& {
        return has_value() ? *this : std::forward<F>(f)();
    }

    template <typename F, typename U = std::invoke_result_t<F>,
              std::enable_if_t<std::is_same_v<U, Option<T>>, int> = 0>
    Option<T> or_else(F&& f) && {
        return has_value() ? std::move(*this) : std::forward<F>(f)();
    }
    /// @}

    /// \brief Takes the value out of the option, leaving a \c None in place
    ///
    /// ```cpp
    /// Option<std::string> opt("hello");
    ///
    /// auto out = opt.take();
    /// assert(out.value() == "hello");
    /// assert(!opt.has_value());
    /// ```
    Option<T> take() & {
        Option<T> opt(std::move(*this));
        return opt;
    }

    /// @{
    /// \brief Returns the pointer to internal storage
    /// \return \c T* or \c nullptr
    ///
    /// If the value is a \c None, \c nullptr is returned
    T* get_pointer() & {
        return has_value() ? std::addressof(storage_.value) : nullptr;
    }

    const T* get_pointer() const& {
        return has_value() ? std::addressof(storage_.value) : nullptr;
    }

    T* get_pointer() && = delete;
    /// @}

    /// \brief Checks whether the option is `Some` or not
    /// \return \c true => Some\n
    ///         \c false => None
    constexpr bool has_value() const noexcept {
        return storage_.has_value;
    }

    /// \brief Checks whether the option is `Some` or not
    /// \return \c true => Some\n
    ///         \c false => None
    /// \see has_value()
    constexpr operator bool() const noexcept {
        return has_value();
    }

    /// @{
    /// \brief Dereference makes it more like a pointer
    /// \throw OptionEmptyException
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

    /// @{
    /// \brief Arrow operator makes it more like a pointer
    /// \throw OptionEmptyException
    constexpr const T* operator->() const {
        return &value();
    }

    constexpr T* operator->() {
        return &value();
    }
    /// @}
private:
    void value_required() const {
        if (!storage_.has_value) {
            throw OptionEmptyException();
        }
    }

    void value_required(const char* s) const {
        if (!storage_.has_value) {
            throw OptionEmptyException(s);
        }
    }

    template <typename... Args>
    void construct(Args&&... args) {
        const void* ptr = std::addressof(storage_.value);
        new (const_cast<void*>(ptr)) T(std::forward<Args>(args)...);
        storage_.has_value = true;
    }

    struct TrivialStorage {
        union {
            T value;
            char dummy;
        };
        bool has_value;

        constexpr TrivialStorage() : dummy('\0'), has_value(false) {}

        void clear() {
            has_value = false;
        }
    };

    struct NonTrivialStorage {
        union {
            T value;
            char dummy;
        };
        bool has_value;

        NonTrivialStorage() : dummy('\0'), has_value(false) {}

        ~NonTrivialStorage() {
            clear();
        }

        void clear() {
            if (has_value) {
                has_value = false;
                value.~T();
            }
        }
    };

    using Storage = std::conditional_t<std::is_trivially_destructible_v<T>,
                                       TrivialStorage, NonTrivialStorage>;
    Storage storage_;
};


/// \brief Swaps between Option object
/// \see Option\<T\>::swap
template <typename T>
void swap(Option<T>& lhs, Option<T>& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
}

/// @{
/// \brief Comparison with others
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
        return lhs.has_value() < rhs.has_value();
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
/// @}

/// @{
/// \brief Comparison with None type, where None is the rhs
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

/// \brief Comparison with None type, where None is the lhs
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
/// @}

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
