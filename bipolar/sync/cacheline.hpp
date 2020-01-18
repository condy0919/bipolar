//! cacheline size
//!
//! `std::hardware_destructive_interference_size` isn't implemented in c++
//!
//! See https://lists.llvm.org/pipermail/cfe-dev/2018-May/058074.html

#ifndef BIPOLAR_SYNC_CACHELINE_HPP_
#define BIPOLAR_SYNC_CACHELINE_HPP_

// Cache line alignment
#if defined(__i386__) || defined(__x86_64__)
#define BIPOLAR_CACHELINE_SIZE 64
#else
#error "Only support x86 x86_64"
#endif

#endif
