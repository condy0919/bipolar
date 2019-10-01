/// \file pipe.hpp
/// \brief operator| for pipable types
/// \see Pipable
/// \see pipe

#ifndef BIPOLAR_ASYNC_PIPE_HPP_
#define BIPOLAR_ASYNC_PIPE_HPP_

#include <type_traits>
#include <utility>

namespace bipolar {
/// \struct Pipable
/// \brief Makes `operator|` visiable to Pipable types by ADL
/// \see https://en.cppreference.com/w/cpp/language/adl
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

/// \brief operator| overload for Pipable types
/// T can be the following types:
/// - Sender
/// - Receiver
/// - Executor
template <typename T, typename Op,
          typename = std::enable_if_t<std::is_invocable_v<Op, T>>>
constexpr auto operator|(T&& t, Op&& op) {
    return op(std::forward<T>(t));
}

/// \struct PipeFn
/// \brief pipe the functions explicitly
/// \see pipe
struct PipeFn {
	template <typename T, typename... Fns>
	constexpr auto operator()(T&& t, Fns&&... fns) const {
		return (std::forward<T>(t) | ... | std::forward<Fns>(fns));
	}
};

/// \var pipe
/// \brief
inline constexpr auto pipe = PipeFn{};

} // namespace bipolar

#endif
