//! Traits
//!
//! Backports some concepts in old style for compatibility
//!
//! see https://en.cppreference.com/w/cpp/header/concepts

#ifndef BIPOLAR_CORE_TRAITS_HPP_
#define BIPOLAR_CORE_TRAITS_HPP_

#include <type_traits>

namespace bipolar {
// order relation
namespace detail {
template <typename, typename, template <typename, typename> typename,
          typename = void>
struct has_relation : std::false_type {};

template <typename T, typename U, template <typename, typename> typename Op>
struct has_relation<T, U, Op, std::enable_if_t<std::is_same_v<Op<T, U>, bool>>>
    : std::true_type {};

template <typename T, typename U, template <typename, typename> typename Op>
inline constexpr bool has_relation_v = has_relation<T, U, Op>::value;

template <typename T, typename U>
using equal_to_t = decltype(std::declval<T>() == std::declval<U>());

template <typename T, typename U>
using not_equal_t = decltype(std::declval<T>() != std::declval<U>());

template <typename T, typename U>
using less_than_t = decltype(std::declval<T>() < std::declval<U>());

template <typename T, typename U>
using less_than_or_equal_to_t = decltype(std::declval<T>() <= std::declval<U>());

template <typename T, typename U>
using greater_than_t = decltype(std::declval<T>() > std::declval<U>());

template <typename T, typename U>
using greater_than_or_equal_to_t = decltype(std::declval<T>() >= std::declval<U>());

// has call operator
template <typename, typename = void>
struct has_call_operator : std::false_type {};

template <typename T>
struct has_call_operator<T, std::void_t<decltype(&T::operator())>>
    : std::true_type {};
} // namespace detail

/// This trait tests equal to and is used by both `operator==` and `operator!=`
template <typename T, typename U = T>
struct is_equality_comparable
    : std::bool_constant<detail::has_relation_v<T, U, detail::equal_to_t> &&
                         detail::has_relation_v<T, U, detail::not_equal_t> &&
                         detail::has_relation_v<U, T, detail::equal_to_t> &&
                         detail::has_relation_v<U, T, detail::not_equal_t>> {};

template <typename T, typename U = T>
inline constexpr bool is_equality_comparable_v = is_equality_comparable<T, U>::value;


/// This trait tests less than and is used by the operator<=
template <typename T, typename U = T>
struct is_less_than_comparable
    : detail::has_relation<T, U, detail::less_than_t> {};

template <typename T, typename U = T>
inline constexpr bool is_less_than_comparable_v =
    is_less_than_comparable<T, U>::value;

/// This trait tests less than or equal to and is used by the `operator<=`
template <typename T, typename U = T>
struct is_less_than_or_equal_to_comparable
    : detail::has_relation<T, U, detail::less_than_or_equal_to_t> {};

template <typename T, typename U = T>
inline constexpr bool is_less_than_or_equal_to_comparable_v =
    is_less_than_or_equal_to_comparable<T, U>::value;

/// This trait tests greater than and is used by the `operator>`
template <typename T, typename U = T>
struct is_greater_than_comparable
    : detail::has_relation<T, U, detail::greater_than_t> {};

template <typename T, typename U = T>
inline constexpr bool is_greater_than_comparable_v =
    is_greater_than_comparable<T, U>::value;

/// This trait tests greater than or equal to and is used by the `operator>=`
template <typename T, typename U = T>
struct is_greater_than_or_equal_to_comparable
    : detail::has_relation<T, U, detail::greater_than_or_equal_to_t> {};

template <typename T, typename U = T>
inline constexpr bool is_greater_than_or_equal_to_comparable_v =
    is_greater_than_or_equal_to_comparable<T, U>::value;

/// This trait tests `T` is strict totally ordered with `U`
template <typename T, typename U = T>
struct is_strict_totally_ordered
    : std::bool_constant<is_equality_comparable_v<T, U> &&
                         is_less_than_comparable_v<T, U> &&
                         is_greater_than_comparable_v<T, U> &&
                         is_less_than_or_equal_to_comparable_v<T, U> &&
                         is_greater_than_or_equal_to_comparable_v<T, U> &&
                         is_less_than_comparable_v<U, T> &&
                         is_greater_than_comparable_v<U, T> &&
                         is_less_than_or_equal_to_comparable_v<U, T> &&
                         is_greater_than_or_equal_to_comparable_v<U, T>> {};

template <typename T, typename U = T>
inline constexpr bool is_strict_totally_ordered_v =
    is_strict_totally_ordered<T, U>::value;

/// This trait tests `T` is callable
///
/// ```
/// assert(is_functor_v<int (*)(int)>);
/// assert(!is_functor_v<int>);
/// ```
template <typename T>
using is_functor = std::bool_constant<std::is_function_v<T> ||
                                      detail::has_call_operator<T>::value>;

template <typename T>
inline constexpr bool is_functor_v = is_functor<T>::value;

/// is_instantiation_of
///
/// Checks if `T` is an instantiation of `C` template
///
/// ```
/// assert((is_instantiation_of_v<std::vector<int>, std::vector>));
/// assert((!is_instantiation_of_v<std::string, std::vector>));
/// ```
template <typename, template <typename...> class>
struct is_instantiation_of : std::false_type {};

template <typename... Ts, template <typename...> class C>
struct is_instantiation_of<C<Ts...>, C> : std::true_type {};

template <typename T, template <typename...> class C>
inline constexpr bool is_instantiation_of_v = is_instantiation_of<T, C>::value;

} // namespace bipolar

#endif
