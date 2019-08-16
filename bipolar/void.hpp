#ifndef BIPOLAR_VOID_HPP_
#define BIPOLAR_VOID_HPP_

namespace bipolar {
/// @class Void
/// @brief The regular void type
/// @see http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2016/p0146r1.html
///
/// It's more like the unit type in functional programming language.
/// Introduce here for facilitating template meta-programming.
struct Void {
    constexpr Void() = default;
    constexpr Void(const Void&) = default;
    constexpr Void& operator=(const Void&) = default;

    template <class T>
    explicit constexpr Void(T&&) noexcept {}
};

constexpr bool operator==(Void, Void) noexcept {
    return true;
}
constexpr bool operator!=(Void, Void) noexcept {
    return false;
}
constexpr bool operator<(Void, Void) noexcept {
    return false;
}
constexpr bool operator<=(Void, Void) noexcept {
    return true;
}
constexpr bool operator>=(Void, Void) noexcept {
    return true;
}
constexpr bool operator>(Void, Void) noexcept {
    return false;
}

} // namespace bipolar

#endif
