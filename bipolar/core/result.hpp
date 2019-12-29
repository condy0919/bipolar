//! Result
//!
//! See `Result` class template

#ifndef BIPOLAR_CORE_RESULT_HPP_
#define BIPOLAR_CORE_RESULT_HPP_

#include <cstdint>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

#include "bipolar/core/traits.hpp"

namespace bipolar {
template <typename T, typename E>
class Result;

/// Pending variant of `Result`
struct Pending {};

/// Ok variant of `Result`
template <typename T>
struct Ok {
    constexpr /*implicit*/ Ok(T&& val) noexcept(
        std::is_nothrow_move_constructible_v<T>)
        : value(std::move(val)) {}

    constexpr /*implicit*/ Ok(const T& val) noexcept(
        std::is_nothrow_copy_constructible_v<T>)
        : value(val) {}

    template <typename E>
    constexpr Result<T, E> into() && {
        return {std::move(*this)};
    }

    T value;
};

/// Err variant of `Result`
template <typename E>
struct Err {
    constexpr /*implicit*/ Err(E&& err) noexcept(
        std::is_nothrow_move_constructible_v<E>)
        : error(std::move(err)) {}

    constexpr /*implicit*/ Err(const E& err) noexcept(
        std::is_nothrow_copy_constructible_v<E>)
        : error(err) {}

    template <typename T>
    constexpr Result<T, E> into() && {
        return {std::move(*this)};
    }

    E error;
};

namespace detail {
/// `Result` related traits:
/// - is_result
/// - is_pending
/// - is_ok
/// - is_error
template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
using is_result = is_instantiation_of<T, Result>;

template <typename T>
inline constexpr bool is_result_v = is_result<T>::value;

template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
using is_pending = std::is_same<T, Pending>;

template <typename T>
inline constexpr bool is_pending_v = is_pending<T>::value;

template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
using is_ok = is_instantiation_of<T, Ok>;

template <typename T>
inline constexpr bool is_ok_v = is_ok<T>::value;

template <typename T>
// NOLINTNEXTLINE(readability-identifier-naming)
using is_error = is_instantiation_of<T, Err>;

template <typename T>
inline constexpr bool is_error_v = is_error<T>::value;

/// Checks whether all `T` meet the `Trait` or not
template <template <typename> typename Trait, typename... Ts>
// NOLINTNEXTLINE(readability-identifier-naming)
using all_of = std::conjunction<Trait<Ts>...>;

template <template <typename> typename Trait, typename... Ts>
inline constexpr bool all_of_v = all_of<Trait, Ts...>::value;
} // namespace detail

/// BadResultAccess
///
/// # Brief
///
/// Throws on logic errors
class BadResultAccess : public std::logic_error {
public:
    BadResultAccess() : std::logic_error("Bad result access") {}

    explicit BadResultAccess(const char* s) : std::logic_error(s) {}
};

/// Result
///
/// # Brief
///
/// `Result` is a type that represents success, failure or pending. And it can
/// be used for returning and propagating errors.
///
/// It's a `std::pair` like class with the variants:
/// - `Ok`, representing success and containing a value
/// - `Err`, representing error and containing an error
/// - `Pending`, representing pending...
///
/// Functions return `Result` whenever errors are expected and recoverable.
///
/// A simple function returning `Result` might be defined and used like so:
///
/// ```
/// Result<int, const char*> parse(const char* s) {
///     if (std::strlen(s) < 3) {
///         return Err("string length is less than 3");
///     }
///     return Ok(s[0] * 100 + s[1] * 10 + s[2]);
/// }
/// ```
///
/// `Result` has similar combinators with `Option`, see document comments for
/// details.
template <typename T, typename E>
class Result final {
    static_assert(!std::is_reference_v<T>,
                  "Result cannot be used with reference type");
    static_assert(!std::is_abstract_v<T>,
                  "Result cannot be used with abstract type");

    template <typename, typename>
    friend class Result;

public:
    using value_type = T; // NOLINT(readability-identifier-naming)
    using error_type = E; // NOLINT(readability-identifier-naming)

    ////////////////////////////////////////////////////////////////////////////
    // Constructors & Assignments
    ////////////////////////////////////////////////////////////////////////////

    /// Constructs an `Pending` variant of `Result`
    constexpr Result() noexcept : sto_(std::in_place_index_t<0>{}) {}
    constexpr /*implicit*/ Result(Pending) noexcept : Result() {}

    /// Constructs from `Ok` variant
    ///
    /// ```
    /// const Result<int, int> x(Ok(42));
    /// assert(x.has_value());
    /// ```
    template <typename U,
              std::enable_if_t<std::is_constructible_v<T, U&&>, int> = 0>
    constexpr /*implicit*/ Result(Ok<U>&& ok) noexcept(
        std::is_nothrow_constructible_v<T, U&&>)
        : sto_(std::in_place_index_t<1>{}, std::move(ok.value)) {}

    template <typename U,
              std::enable_if_t<std::is_constructible_v<T, U>, int> = 0>
    constexpr /*implicit*/ Result(const Ok<U>& ok) noexcept(
        std::is_nothrow_constructible_v<T, U>)
        : sto_(std::in_place_index_t<1>{}, ok.value) {}

    /// Constructs from `Err` variant
    ///
    /// ```
    /// const Result<int, int> x(Err(42));
    /// assert(x.has_error());
    /// ```
    template <typename U,
              std::enable_if_t<std::is_constructible_v<E, U&&>, int> = 0>
    constexpr /*implicit*/ Result(Err<U>&& err) noexcept(
        std::is_nothrow_constructible_v<E, U&&>)
        : sto_(std::in_place_index_t<2>{}, std::move(err.error)) {}

    template <typename U,
              std::enable_if_t<std::is_constructible_v<E, U>, int> = 0>
    constexpr /*implicit*/ Result(const Err<U>& err) noexcept(
        std::is_nothrow_constructible_v<E, U>)
        : sto_(std::in_place_index_t<2>{}, err.error) {}

    /// Constructs from other type `Result<X, Y>` where
    /// `T` can be constructible with `X` and
    /// `E` can be constructible with `Y`
    template <typename X, typename Y,
              std::enable_if_t<!std::is_same_v<Result<X, Y>, Result<T, E>> &&
                                   std::is_constructible_v<T, X> &&
                                   std::is_constructible_v<E, Y>,
                               int> = 0>
    constexpr Result(const Result<X, Y>& rhs) noexcept(
        std::is_nothrow_constructible_v<T, X>&&
            std::is_nothrow_constructible_v<E, Y>) {
        switch (rhs.sto_.index()) {
        case 0:
            sto_.template emplace<0>();
            break;

        case 1:
            sto_.template emplace<1>(std::get<1>(rhs.sto_));
            break;

        case 2:
            sto_.template emplace<2>(std::get<2>(rhs.sto_));
            break;
        }
    }

    template <typename X, typename Y,
              std::enable_if_t<!std::is_same_v<Result<X, Y>, Result<T, E>> &&
                                   std::is_constructible_v<T, X&&> &&
                                   std::is_constructible_v<E, Y&&>,
                               int> = 0>
    constexpr Result(Result<X, Y>&& rhs) noexcept(
        std::is_nothrow_constructible_v<T, X&&>&&
            std::is_nothrow_constructible_v<E, Y&&>) {
        switch (rhs.sto_.index()) {
        case 0:
            sto_.template emplace<0>();
            break;

        case 1:
            sto_.template emplace<1>(std::move(std::get<1>(rhs.sto_)));
            break;

        case 2:
            sto_.template emplace<2>(std::move(std::get<2>(rhs.sto_)));
            break;
        }
    }

    /// Constructs from other results, leaving it pending
    constexpr Result(Result&& rhs) noexcept(
        detail::all_of_v<std::is_nothrow_move_constructible, T, E>)
        : sto_(std::move(rhs.sto_)) {
        rhs.sto_.template emplace<0>();
    }

    constexpr Result& operator=(Result&& rhs) noexcept(
        detail::all_of_v<std::is_nothrow_move_assignable, T, E>) {
        sto_ = std::move(rhs.sto_);
        rhs.sto_.template emplace<0>();
        return *this;
    }

    /// Copy/Assigns if copyable/assignable
    constexpr Result(const Result&) = default;
    constexpr Result& operator=(const Result&) = default;

    /// Assigns a pending
    constexpr Result& operator=(Pending) noexcept {
        assign(Pending{});
        return *this;
    }

    /// Assigns an ok
    template <typename U,
              std::enable_if_t<std::is_constructible_v<T, U>, int> = 0>
    constexpr Result&
    operator=(const Ok<U>& ok) noexcept(std::is_nothrow_constructible_v<T, U>) {
        assign(ok);
        return *this;
    }

    template <typename U,
              std::enable_if_t<std::is_constructible_v<T, U&&>, int> = 0>
    constexpr Result&
    operator=(Ok<U>&& ok) noexcept(std::is_nothrow_constructible_v<T, U&&>) {
        assign(std::move(ok));
        return *this;
    }

    /// Assigns an error
    template <typename U,
              std::enable_if_t<std::is_constructible_v<E, U>, int> = 0>
    constexpr Result& operator=(const Err<U>& err) noexcept(
        std::is_nothrow_constructible_v<E, U>) {
        assign(err);
        return *this;
    }

    template <typename U,
              std::enable_if_t<std::is_constructible_v<E, U&&>, int> = 0>
    constexpr Result&
    operator=(Err<U>&& err) noexcept(std::is_nothrow_constructible_v<E, U&&>) {
        assign(std::move(err));
        return *this;
    }

    /// Assigns with `Pending` type
    constexpr void assign(Pending) noexcept {
        sto_.template emplace<0>();
    }

    /// Assigns with `Ok` type
    template <typename U,
              std::enable_if_t<std::is_constructible_v<T, U>, int> = 0>
    constexpr void
    assign(const Ok<U>& ok) noexcept(std::is_nothrow_constructible_v<T, U>) {
        sto_.template emplace<1>(ok.value);
    }

    template <typename U,
              std::enable_if_t<std::is_constructible_v<T, U&&>, int> = 0>
    constexpr void
    assign(Ok<U>&& ok) noexcept(std::is_nothrow_constructible_v<T, U&&>) {
        sto_.template emplace<1>(std::move(ok.value));
    }

    /// Assigns with `Err` type
    template <typename U,
              std::enable_if_t<std::is_constructible_v<E, U>, int> = 0>
    constexpr void
    assign(const Err<U>& err) noexcept(std::is_nothrow_constructible_v<E, U>) {
        sto_.template emplace<2>(err.error);
    }

    template <typename U,
              std::enable_if_t<std::is_constructible_v<E, U&&>, int> = 0>
    constexpr void
    assign(Err<U>&& err) noexcept(std::is_nothrow_constructible_v<E, U&&>) {
        sto_.template emplace<2>(std::move(err.error));
    }

    ////////////////////////////////////////////////////////////////////////////
    // Combinators
    ////////////////////////////////////////////////////////////////////////////

    /// Maps a `Result<T, E>` to `Result<U, E>` by applying a function
    /// to the contained `Ok` value.
    /// Has no effect With `Pending` or `Err`.
    ///
    /// # Constraints
    ///
    /// F :: T -> U
    ///
    /// # Examples
    ///
    /// ```
    /// const Result<int, int> x(Ok(2));
    /// const auto res = x.map([](int x) { return x + 1; });
    /// assert(res.is_ok() && res.value() == 3);
    /// ```
    template <typename F, typename U = std::invoke_result_t<F, T>>
    constexpr Result<U, E> map(F&& f) const& {
        switch (sto_.index()) {
        case 0:
            return Pending{};

        case 1:
            return Ok(std::forward<F>(f)(std::get<1>(sto_)));

        case 2:
            return Err(std::get<2>(sto_));
        }
        __builtin_unreachable();
    }

    template <typename F, typename U = std::invoke_result_t<F, T>>
    constexpr Result<U, E> map(F&& f) && {
        switch (sto_.index()) {
        case 0:
            return Pending{};

        case 1:
            return Ok(std::forward<F>(f)(std::move(std::get<1>(sto_))));

        case 2:
            return Err(std::move(std::get<2>(sto_)));
        }
        __builtin_unreachable();
    }

    /// Maps a Result<T, E> to U by applying a function to the contained
    /// `Ok` value, or a fallback function to a contained `Err` value.
    /// If it's `Pending`, throws `BadResultAccess` exception.
    ///
    /// # Constraints
    ///
    /// F :: T -> U
    /// M :: E -> U
    ///
    /// # Examples
    ///
    /// ```
    /// const int k = 21;
    ///
    /// const Result<int, int> x(Ok(2));
    /// const auto res = x.map_or_else([k]() { return 2 * k; },
    ///                                [](int x) { return x; });
    /// assert(res == 2);
    ///
    /// const Result<int, int> y(Err(2));
    /// const auto res2 = y.map_or_else([k]() { return 2 * k; },
    ///                                 [](int x) { return x; });
    /// assert(res2 == 42);
    /// ```
    template <typename M, typename F, typename U = std::invoke_result_t<F, T>,
              std::enable_if_t<std::is_same_v<U, std::invoke_result_t<M, E>>,
                               int> = 0>
    constexpr U map_or_else(M&& fallback, F&& f) const& {
        nonpending_required();
        return is_ok() ? std::forward<F>(f)(std::get<1>(sto_))
                       : std::forward<M>(fallback)(std::get<2>(sto_));
    }

    template <typename M, typename F, typename U = std::invoke_result_t<F, T>,
              std::enable_if_t<std::is_same_v<U, std::invoke_result_t<M, E>>,
                               int> = 0>
    constexpr U map_or_else(M&& fallback, F&& f) && {
        nonpending_required();
        return is_ok()
                   ? std::forward<F>(f)(std::move(std::get<1>(sto_)))
                   : std::forward<M>(fallback)(std::move(std::get<2>(sto_)));
    }

    /// Maps a `Result<T, E>` to `Result<T, U>` by applying a function
    /// to the contained `Err` value, leaving an `Ok`/`Pending` value untouched.
    ///
    /// # Constraints
    ///
    /// F :: E -> U
    ///
    /// # Examples
    ///
    /// ```
    /// const Result<int, int> x(Ok(2));
    /// const auto res = x.map_err([](int x) { return x + 1; });
    /// assert(!res.has_error());
    ///
    /// const Result<int, int> y(Err(3));
    /// const auto res2 = y.map_err([](int x) { return x + 1; });
    /// assert(res2.is_error() && res2.error() == 4);
    /// ```
    template <typename F, typename U = std::invoke_result_t<F, E>>
    constexpr Result<T, U> map_err(F&& f) const& {
        switch (sto_.index()) {
        case 0:
            return Pending{};

        case 1:
            return Ok(std::get<1>(sto_));

        case 2:
            return Err(std::forward<F>(f)(std::get<2>(sto_)));
        }
        __builtin_unreachable();
    }

    template <typename F, typename U = std::invoke_result_t<F, E>>
    constexpr Result<T, U> map_err(F&& f) && {
        switch (sto_.index()) {
        case 0:
            return Pending{};

        case 1:
            return Ok(std::move(std::get<1>(sto_)));

        case 2:
            return Err(std::forward<F>(f)(std::move(std::get<2>(sto_))));
        }
        __builtin_unreachable();
    }

    /// Calls `f` if the result is `Ok`, otherwise returns the `Err` value of
    /// self.
    /// If it's `Pending`, throws `BadResultAccess` exception.
    ///
    /// # Constraints
    ///
    /// F :: T -> U
    ///
    /// # Examples
    ///
    /// ```
    /// Result<int, int> sq(int x) {
    ///     return Ok(x * x);
    /// }
    ///
    /// Result<int, int> err(int x) {
    ///     return Err(x);
    /// }
    ///
    /// const Result<int, int> x(Ok(2));
    /// assert(x.and_then(sq).and_then(sq) == Ok(16));
    /// assert(x.and_then(sq).and_then(err) == Err(4));
    /// assert(x.and_then(err).and_then(sq) == Err(2));
    /// assert(x.and_then(err).and_then(err) == Err(2));
    /// ```
    template <typename F, typename U = std::invoke_result_t<F, T>,
              std::enable_if_t<detail::is_result_v<U> &&
                                   std::is_same_v<typename U::error_type, E>,
                               int> = 0>
    constexpr U and_then(F&& f) const& {
        nonpending_required();
        return is_ok() ? std::forward<F>(f)(std::get<1>(sto_))
                       : static_cast<U>(Err(std::get<2>(sto_)));
    }

    template <typename F, typename U = std::invoke_result_t<F, T>,
              std::enable_if_t<detail::is_result_v<U> &&
                                   std::is_same_v<typename U::error_type, E>,
                               int> = 0>
    constexpr U and_then(F&& f) && {
        nonpending_required();
        return is_ok() ? std::forward<F>(f)(std::move(std::get<1>(sto_)))
                       : static_cast<U>(Err(std::move(std::get<2>(sto_))));
    }

    /// Calls `f` if the result is `Err`, otherwise returns the `Ok` value of
    /// self.
    /// If it's `Pending`, throws `BadResultAccess` exception.
    ///
    /// # Constraints
    ///
    /// F :: E -> U
    ///
    /// # Examples
    ///
    /// ```
    /// Result<int, int> sq(int x) {
    ///     return Ok(x * x);
    /// }
    ///
    /// Result<int, int> err(int x) {
    ///     return Err(x);
    /// }
    ///
    /// const Result<int, int> x(Ok(2)), y(Err(3));
    /// assert(x.or_else(sq).or_else(sq) == Ok(2));
    /// assert(x.or_else(err).or_else(sq) == Ok(2));
    /// assert(y.or_else(sq).or_else(err) == Ok(9));
    /// assert(y.or_else(err).or_else(err) == Err(3));
    /// ```
    template <typename F, typename U = std::invoke_result_t<F, E>,
              std::enable_if_t<detail::is_result_v<U> &&
                                   std::is_same_v<typename U::value_type, T>,
                               int> = 0>
    constexpr U or_else(F&& f) const& {
        nonpending_required();
        return is_ok() ? static_cast<U>(Ok(std::get<1>(sto_)))
                       : std::forward<F>(f)(std::get<2>(sto_));
    }

    template <typename F, typename U = std::invoke_result_t<F, E>,
              std::enable_if_t<detail::is_result_v<U> &&
                                   std::is_same_v<typename U::value_type, T>,
                               int> = 0>
    constexpr U or_else(F&& f) && {
        nonpending_required();
        return is_ok() ? static_cast<U>(Ok(std::move(std::get<1>(sto_))))
                       : std::forward<F>(f)(std::move(std::get<2>(sto_)));
    }

    ///////////////////////////////////////////////////////////////////////////
    // Utilities
    ///////////////////////////////////////////////////////////////////////////

    /// Returns `true` if the result is an `Ok` value containing the
    /// given value.
    ///
    /// ```
    /// const Result<int, int> x(Ok(2));
    /// assert(x.contains(2));
    /// assert(!x.contains(3));
    ///
    /// const Result<int, int> y(Err(2));
    /// assert(!y.contains(2));
    /// ```
    template <typename U,
              std::enable_if_t<is_equality_comparable_v<T, U>, int> = 0>
    constexpr bool contains(const U& x) const noexcept {
        return is_ok() ? std::get<1>(sto_) == x : false;
    }

    /// Returns `true` if the result is an `Err` value containing the
    /// given value.
    ///
    /// ```
    /// const Result<int, int> x(Ok(2));
    /// assert(!x.contains_err(2));
    ///
    /// const Result<int, int> y(Err(2));
    /// assert(y.contains_err(2));
    ///
    /// const Result<int, int> z(Err(3));
    /// assert(!z.contains_err(2));
    /// ```
    template <typename U,
              std::enable_if_t<is_equality_comparable_v<E, U>, int> = 0>
    constexpr bool contains_err(const U& x) const noexcept {
        return is_error() ? std::get<2>(sto_) == x : false;
    }

    /// Inplacement constructs from args
    ///
    /// ```
    /// Result<std::string, int> x(Err(2));
    /// assert(x.has_error() && x.error() == 2);
    ///
    /// x.emplace("foo");
    /// assert(x.has_value() && x.value() == "foo");
    /// ```
    template <typename... Args>
    constexpr T& emplace(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...>) {
        sto_.template emplace<1>(std::forward<Args>(args)...);
        return std::get<1>(sto_);
    }

    /// Unwraps a result, yielding the content of an `Ok`.
    /// Else, it returns `deft`
    ///
    /// Arguments passed to `value_or` are eagerly evaluated; if you are
    /// passing the result of a function call, it is recommended to use
    /// `value_or_else`, which is lazily evaluated.
    ///
    /// ```
    /// const Result<int, int> x(Ok(2));
    /// assert(x.value_or(3) == 2);
    ///
    /// const Result<int, int> y(Err(2));
    /// assert(y.value_or(3) == 3);
    /// ```
    template <typename U>
    constexpr T value_or(U&& deft) const& {
        return is_ok() ? std::get<1>(sto_)
                       : static_cast<T>(std::forward<U>(deft));
    }

    template <typename U>
    constexpr T value_or(U&& deft) && {
        return is_ok() ? std::move(std::get<1>(sto_))
                       : static_cast<T>(std::forward<U>(deft));
    }

    /// Unwraps a result, yielding the content of an `Ok`
    /// If the value is an `Err`, then it calls `f` with its `Err`.
    /// Otherwise throws a `BadResultAccess` exception.
    ///
    /// # Constraints
    ///
    /// F :: E -> T
    ///
    /// # Examples
    ///
    /// ```
    /// const Result<int, int> x(Ok(2));
    /// const int resx = x.value_or_else([](int x) { return x; });
    /// assert(resx == 2);
    ///
    /// const Result<int, int> y(Err(3));
    /// const int resy = y.value_or_else([](int y) { return y; });
    /// assert(resy == 3);
    /// ```
    template <typename F,
              std::enable_if_t<std::is_same_v<std::invoke_result_t<F, E>, T>,
                               int> = 0>
    constexpr T value_or_else(F&& f) const& {
        nonpending_required();
        return is_ok() ? std::get<1>(sto_)
                       : std::forward<F>(f)(std::get<2>(sto_));
    }

    template <typename F,
              std::enable_if_t<std::is_same_v<std::invoke_result_t<F, E>, T>,
                               int> = 0>
    constexpr T value_or_else(F&& f) && {
        nonpending_required();
        return is_ok() ? std::move(std::get<1>(sto_))
                       : std::forward<F>(f)(std::move(std::get<2>(sto_)));
    }

    /// Resets to `Pending` state
    constexpr void reset() {
        assign(Pending{});
    }

    /// Unwraps a result, yielding the content of an `Ok`
    ///
    /// Throws BadResultAccess when the result is `Err`
    constexpr T& value() & {
        value_required();
        return std::get<1>(sto_);
    }

    constexpr const T& value() const& {
        value_required();
        return std::get<1>(sto_);
    }

    constexpr T&& value() && {
        value_required();
        return std::move(std::get<1>(sto_));
    }

    constexpr const T&& value() const&& {
        value_required();
        return std::move(std::get<1>(sto_));
    }

    /// Takes the result's value, leaving it in a pending state
    ///
    /// Throws `BadResultAccess` when not `Ok`
    constexpr T take_value() {
        auto v = std::move(value());
        reset();
        return v;
    }

    /// Unwraps a result, yielding the content of an `Ok`
    ///
    /// Throws BadResultAccess with description when the result is `Err`
    constexpr const T& expect(const char* s) const& {
        value_required(s);
        return std::get<1>(sto_);
    }

    constexpr T&& expect(const char* s) && {
        value_required(s);
        return std::move(std::get<1>(sto_));
    }

    /// Unwraps a result, yielding the content of an `Err`
    ///
    /// Throws BadResultAccess when the result is `Ok`
    constexpr E& error() & {
        error_required();
        return std::get<2>(sto_);
    }

    constexpr const E& error() const& {
        error_required();
        return std::get<2>(sto_);
    }

    constexpr E&& error() && {
        error_required();
        return std::move(std::get<2>(sto_));
    }

    constexpr const E&& error() const&& {
        error_required();
        return std::move(std::get<2>(sto_));
    }

    /// Takes the result's error, leaving it in a pending state
    ///
    /// Throws `BadResultAccess` when not `Err`
    constexpr E take_error() {
        auto e = std::move(error());
        reset();
        return e;
    }

    /// Unwraps a result, yielding the content of an `Err`
    ///
    /// Throws BadResultAccess with description when the result is `Ok`
    constexpr const E& expect_err(const char* s) const& {
        error_required(s);
        return std::get<2>(sto_);
    }

    constexpr const E&& expect_err(const char* s) && {
        error_required(s);
        return std::move(std::get<2>(sto_));
    }

    /// Returns `true` if the result is `Pending` variant
    ///
    /// ```
    /// Result<int, int> res;
    /// assert(res.is_pending());
    ///
    /// Result<int, int> x = Ok(3);
    /// assert(!res.is_pending());
    /// ```
    [[nodiscard]] constexpr bool is_pending() const noexcept {
        return sto_.index() == 0;
    }

    /// Returns `true` if the result is `Ok` variant
    ///
    /// ```
    /// Result<int, int> res = Ok(3);
    /// assert(res.is_ok());
    ///
    /// Result<int, int> err_res = Err(3);
    /// assert(!res.is_ok());
    /// ```
    [[nodiscard]] constexpr bool is_ok() const noexcept {
        return sto_.index() == 1;
    }

    /// Returns `true` if the result is `Err` variant
    ///
    /// ```
    /// Result<int, int> err_res = Err(3);
    /// assert(err_res.is_error());
    /// ```
    [[nodiscard]] constexpr bool is_error() const noexcept {
        return sto_.index() == 2;
    }

    /// Checks if the result is not `Pending`
    constexpr explicit operator bool() const noexcept {
        return sto_.index() != 0;
    }

    /// Dereference makes it more like a pointer
    ///
    /// Throws `BadResultAccess` when the result is `Err`
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
    ///
    /// Throws `BadResultAccess` when the result is `Err`
    constexpr const T* operator->() const {
        return std::addressof(value());
    }

    constexpr T* operator->() {
        return std::addressof(value());
    }

    /// Swaps with other results
    constexpr void swap(Result& rhs) noexcept(
        detail::all_of_v<std::is_nothrow_swappable, T, E>) {
        std::swap(sto_, rhs.sto_);
    }

private:
    constexpr void nonpending_required() const {
        if (is_pending()) {
            throw BadResultAccess();
        }
    }

    constexpr void value_required() const {
        if (!is_ok()) {
            throw BadResultAccess();
        }
    }

    constexpr void value_required(const char* s) const {
        if (!is_ok()) {
            throw BadResultAccess(s);
        }
    }

    constexpr void error_required() const {
        if (!is_error()) {
            throw BadResultAccess();
        }
    }

    constexpr void error_required(const char* s) const {
        if (!is_error()) {
            throw BadResultAccess(s);
        }
    }

    std::variant<std::monostate, T, E> sto_;
};

/// Swaps Results
///
/// see `Result<T, E>::swap` for details
template <typename T, typename E>
inline constexpr void swap(Result<T, E>& lhs, Result<T, E>& rhs) noexcept(
    detail::all_of_v<std::is_nothrow_swappable, T, E>) {
    lhs.swap(rhs);
}

/// Comparison with others
template <
    typename T, typename E,
    std::enable_if_t<is_equality_comparable_v<T> && is_equality_comparable_v<E>,
                     int> = 0>
inline constexpr bool operator==(const Result<T, E>& lhs,
                                 const Result<T, E>& rhs) {
    if (lhs.is_ok() && rhs.is_ok()) {
        return lhs.value() == rhs.value();
    } else if (lhs.is_error() == rhs.is_error()) {
        return lhs.error() == rhs.error();
    }
    return lhs.is_pending() == rhs.is_pending();
}

template <
    typename T, typename E,
    std::enable_if_t<is_equality_comparable_v<T> && is_equality_comparable_v<E>,
                     int> = 0>
inline constexpr bool operator!=(const Result<T, E>& lhs,
                                 const Result<T, E>& rhs) {
    return !(lhs == rhs);
}

/// Specialized comparisons with `Ok` Type
/// `Ok` is in the right
template <typename T, typename E,
          std::enable_if_t<is_equality_comparable_v<T>, int> = 0>
inline constexpr bool operator==(const Result<T, E>& lhs, const Ok<T>& rhs) {
    return lhs.is_ok() && lhs.value() == rhs.value;
}

template <typename T, typename E,
          std::enable_if_t<is_equality_comparable_v<T>, int> = 0>
inline constexpr bool operator!=(const Result<T, E>& lhs, const Ok<T>& rhs) {
    return !(lhs == rhs);
}

/// Specialized comparisons with `Ok` Type
/// `Ok` is in the left
template <typename T, typename E,
          std::enable_if_t<is_equality_comparable_v<T>, int> = 0>
inline constexpr bool operator==(const Ok<T>& lhs, const Result<T, E>& rhs) {
    return rhs == lhs;
}

template <typename T, typename E,
          std::enable_if_t<is_equality_comparable_v<T>, int> = 0>
inline constexpr bool operator!=(const Ok<T>& lhs, const Result<T, E>& rhs) {
    return rhs != lhs;
}

/// Specialized comparisons with `Err` Type
/// `Err` is in the right
template <typename T, typename E,
          std::enable_if_t<is_equality_comparable_v<E>, int> = 0>
inline constexpr bool operator==(const Result<T, E>& lhs, const Err<E>& rhs) {
    return lhs.is_error() && lhs.error() == rhs.error;
}

template <typename T, typename E,
          std::enable_if_t<is_equality_comparable_v<E>, int> = 0>
inline constexpr bool operator!=(const Result<T, E>& lhs, const Err<E>& rhs) {
    return !(lhs == rhs);
}

/// Specialized comparisons with `Err` Type
/// `Err` is in the left
template <typename T, typename E,
          std::enable_if_t<is_equality_comparable_v<E>, int> = 0>
inline constexpr bool operator==(const Err<E>& lhs, const Result<T, E>& rhs) {
    return rhs == lhs;
}

template <typename T, typename E,
          std::enable_if_t<is_equality_comparable_v<E>, int> = 0>
inline constexpr bool operator!=(const Err<E>& lhs, const Result<T, E>& rhs) {
    return rhs != lhs;
}

} // namespace bipolar

#endif
