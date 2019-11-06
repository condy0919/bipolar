//! The assume macro
//!
//! # Brief
//!
//! The boolean argument to this function is defined to be true.
//! The optimizer may analyze the form of the expression provided as the
//! argument and deduce from that information used to optimize the program.
//! If the condition is violated during execution, the behavior is undefined.
//! The argument itself is never evaluated, so any side effects of the
//! expression will be discarded.
//!
//! # Examples
//!
//! ```
//! int divide_by_32(int x) {
//!     BIPOLAR_ASSUME(x >= 0);
//!     return x / 32;
//! }
//! ```
//!
//! # Reference
//!
//! http://wg21.link/p1774r0
//! https://clang.llvm.org/docs/LanguageExtensions.html#builtin-assume

#ifndef BIPOLAR_CORE_ASSUME_HPP_
#define BIPOLAR_CORE_ASSUME_HPP_

#if defined(__clang__)
#define BIPOLAR_ASSUME(exp) __builtin_assume((exp))
#elif defined(__GNUC__)
#define BIPOLAR_ASSUME(exp) (void)((exp) ? 0 : (__builtin_unreachable(), 0))
#else
#define BIPOLAR_ASSUME(exp)
#endif

#endif
