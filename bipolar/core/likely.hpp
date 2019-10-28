//! The wellknown likely/unlikely macros
//!
//! # Brief
//!
//! The use of the `likely` macro is intended to allow compilers to optimize for
//! the good path.
//!
//! The use of the `unlikely` macro is intended to allow compilers to optimize
//! for bad path.
//!
//! All allows compilers to optimize code layout to be more cache friendly.
//!
//! All require a `bool` value or else compile error
//!
//! Excessive usage of either of these macros is liable to result in performance
//! degradation.
//!
//! # Examples
//!
//! Basic use:
//!
//! ```
//! if (BIPOLAR_LIKELY(foo())) {
//!     // do sth...
//! }
//! ```
//!
//! # Reference
//!
//! http://wg21.link/p0479r5
//! https://lwn.net/Articles/255364/

#ifndef BIPOLAR_CORE_LIKELY_HPP_
#define BIPOLAR_CORE_LIKELY_HPP_

#ifdef __GNUC__
#define BIPOLAR_BUILTIN_EXPECT(exp, c) __builtin_expect((exp), (c))
#else
#define BIPOLAR_BUILTIN_EXPECT(exp, c) (exp)
#endif

#define BIPOLAR_LIKELY(exp) BIPOLAR_BUILTIN_EXPECT((exp), true)
#define BIPOLAR_UNLIKELY(exp) BIPOLAR_BUILTIN_EXPECT((exp), false)

#endif
