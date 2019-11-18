//! Movable
//!
//! `Movable` is like `boost::noncopyable`.
//! But it allows move constructible/assignable.

#ifndef BIPOLAR_CORE_MOVABLE_HPP_
#define BIPOLAR_CORE_MOVABLE_HPP_

namespace bipolar {
class Movable {
public:
    constexpr Movable() noexcept = default;
    constexpr Movable(Movable&&) noexcept = default;
    constexpr Movable& operator=(Movable&&) noexcept = default;

    Movable(const Movable&) = delete;
    Movable& operator=(const Movable&) = delete;

    ~Movable() = default;
};
} // namespace bipolar

#endif
