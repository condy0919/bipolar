//! The hash algorithm collection
//!

#ifndef BIPOLAR_CORE_HASH_HPP_
#define BIPOLAR_CORE_HASH_HPP_

#include <cstdint>

namespace bipolar {
namespace detail {
template <std::uint64_t>
struct FibHelper;

template <>
struct FibHelper<0> {
    static constexpr int log2 = -1;
};

template <std::uint64_t N>
struct FibHelper {
    static constexpr int log2 = FibHelper<N / 2>::log2 + 1;
    static constexpr std::uint64_t shift_bits = 64 - log2;
};
} // namespace detail

/// Fibonacci hashing
///
/// Based on https://probablydance.com/2018/06/16/fibonacci-hashing-the-optimization-that-the-world-forgot-or-a-better-alternative-to-integer-modulo/
template <std::size_t N>
constexpr std::uint64_t fibhash(std::uint64_t x) {
    static_assert(N != 0, "N must not be zero");
    static_assert((N & (N - 1)) == 0, "N must be power of 2");

    constexpr std::uint64_t shift_bits = detail::FibHelper<N>::shift_bits;
    return (0x9e3779b97f4a7c13UL * (x ^ (x >> shift_bits))) >> shift_bits;
}
} // namespace bipolar

#endif
