//! Void
//!
//! # Brief
//!
//! The regular [`void`] type
//!
//! It's more like the unit type in functional programming language.
//! Introduce here for facilitating template meta-programming.
//!
//! [`void`]: http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2016/p0146r1.html

#ifndef BIPOLAR_CORE_VOID_HPP_
#define BIPOLAR_CORE_VOID_HPP_

namespace bipolar {
struct Void {
    constexpr Void() noexcept = default;
    constexpr Void(const Void&) noexcept = default;
    constexpr Void& operator=(const Void&) noexcept = default;
    constexpr Void(Void&&) noexcept = default;
    constexpr Void& operator=(Void&&) noexcept = default;

    // It shadows all constructors including move and copy.
    //
    // See the story behind the code
    // https://github.com/condy0919/bipolar/issues/21
    template <typename T>
    constexpr explicit Void(T&&) noexcept {} // NOLINT
};

inline constexpr bool operator==(Void, Void) noexcept {
    return true;
}
inline constexpr bool operator!=(Void, Void) noexcept {
    return false;
}
inline constexpr bool operator<(Void, Void) noexcept {
    return false;
}
inline constexpr bool operator<=(Void, Void) noexcept {
    return true;
}
inline constexpr bool operator>=(Void, Void) noexcept {
    return true;
}
inline constexpr bool operator>(Void, Void) noexcept {
    return false;
}

} // namespace bipolar

#endif
