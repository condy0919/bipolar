//! Pipe
//!
//! # Brief
//!
//! Pipe introduces composability which allows Senders, Receivers,
//! and Executors to compose.
//!
//! Composability involves creating functions in the pattern
//! `Executor(Executor)` and `Continuation(Continuation)` that adapt one
//! implementation into another while composingsome additional functionality.
//!
//! Composability involves creating functions that compose sets of adaptors in
//! the pattern of `Executor operator|(Executor, Executor(Executor))` and
//! `Executor pipe(Executor, Executor(Executor)...)`.
//!
//! # Examples
//!
//! see `Pipable` and `pipe` for examples

#ifndef BIPOLAR_ASYNC_PIPE_HPP_
#define BIPOLAR_ASYNC_PIPE_HPP_

#include <utility>
#include <type_traits>

namespace bipolar {
/// The `Pipable` base class
///
/// It makes `operator|` visiable to Pipable types by ADL.
/// see https://en.cppreference.com/w/cpp/language/adl for details.
///
/// # Examples
///
/// ```
/// struct foo : Pipable {};
///
/// void test(foo) {}
///
/// foo() | test;
/// ```
struct Pipable {};

/// `operator|` for pipable types
///
/// T can be the following types:
/// - Sender
/// - Receiver
/// - Executor
template <typename T, typename Op,
          typename = std::enable_if_t<std::is_invocable_v<Op, T>>>
constexpr auto operator|(T&& t, Op&& op) {
    return op(std::forward<T>(t));
}

/// The `pipe` functor
///
/// Same with `t | op0 | op1 | op2 | ... | op`
struct PipeFn {
    template <typename T, typename... Fns>
    constexpr auto operator()(T&& t, Fns&&... fns) const {
        return (std::forward<T>(t) | ... | std::forward<Fns>(fns));
    }
};

inline constexpr PipeFn pipe{};

} // namespace bipolar

#endif
