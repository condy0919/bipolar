//! Overload trick
//!
//! Copy from
//! https://dev.to/tmr232/that-overloaded-trick-overloading-lambdas-in-c17
//!
//! # Examples
//!
//! ```
//! std::variant<int, const char*> var{"hello"};
//!
//! std::visit(
//!     Overload{
//!         [](const char* s) { puts(s); },
//!         [](int x) { printf("%d\n", x); },
//!     },
//!     var);
//! ```

#ifndef BIPOLAR_CORE_OVERLOAD_HPP_
#define BIPOLAR_CORE_OVERLOAD_HPP_

namespace bipolar {
template <typename... Fns>
struct Overload : Fns... {
    using Fns::operator()...;
};

template <typename... Fns>
Overload(Fns...) -> Overload<Fns...>;

} // namespace bipolar

#endif
