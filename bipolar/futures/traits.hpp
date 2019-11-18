//! Futures module traits
//!
//! Traits about:
//! - Continuation
//! - AsyncResult (internal only)
//!

#ifndef BIPOLAR_FUTURES_TRAITS_HPP_
#define BIPOLAR_FUTURES_TRAITS_HPP_

#include <type_traits>

#include "bipolar/futures/async_result.hpp"

namespace bipolar {
// forward
class Context;

/// continuation_traits
///
/// Deduces a continuation's result.
/// Also ensures that the continuation has a complete signature:
/// `AsyncResult<T, E>(Context&)`
///
/// ```
/// using C = AsyncResult<int, int>(Context&);
/// assert(is_continuation_v<C>);
/// ```
template <typename Continuation,
          std::enable_if_t<detail::is_async_result_v<
                               std::invoke_result_t<Continuation, Context&>>,
                           int> = 0>
struct continuation_traits {
    using type = Continuation;
    using result_type = std::invoke_result_t<Continuation, Context&>;
};

/// Checks if it's a continuation type
template <typename Continuation, typename = void>
struct is_continuation : std::false_type {};

template <typename Continuation>
struct is_continuation<
    Continuation, std::void_t<typename continuation_traits<Continuation>::type>>
    : std::true_type {};

template <typename Continuation>
inline constexpr bool is_continuation_v = is_continuation<Continuation>::value;

} // namespace bipolar

#endif
