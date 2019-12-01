//! EnableSpecialMembers
//!
//! internal use only

#ifndef BIPOLAR_CORE_INTERNAL_ENABLE_SPECIAL_MEMBERS_HPP_
#define BIPOLAR_CORE_INTERNAL_ENABLE_SPECIAL_MEMBERS_HPP_

namespace bipolar {
namespace internal {

/// EnableDefaultConstructor
///
/// A mixin helper to conditionally enable the default constructor
template <bool Enable, bool Noexcept = true, typename Tag = void>
struct EnableDefaultConstructor;

/// EnableDestructor
///
/// A mixin helper to conditionally enable the destructor
template <bool Enable, bool Noexcept = true, typename Tag = void>
struct EnableDestructor;

/// EnableCopyConstructor
///
/// A mixin helper to conditionally enable copy constructor
template <bool Enable, bool Noexcept = true, typename Tag = void>
struct EnableCopyConstructor;

/// EnableCopyAssignment
///
/// A mixin helper to conditionally enable copy assignment
template <bool Enable, bool Noexcept = true, typename Tag = void>
struct EnableCopyAssignment;

/// EnableMoveConstructor
///
/// A mixin helper to conditionally enable move constructor
template <bool Enable, bool Noexcept = true, typename Tag = void>
struct EnableMoveConstructor;

/// EnableMoveAssignment
///
/// A mixin helper to conditionally enable move assignment
template <bool Enable, bool Noexcept = true, typename Tag = void>
struct EnableMoveAssignment;

// clang-format off

// Boilerplate follows

// EnableDefaultConstructor specialization
template <bool Noexcept, typename Tag>
struct EnableDefaultConstructor<false, Noexcept, Tag> {
    constexpr EnableDefaultConstructor() noexcept                                           = delete;
    constexpr EnableDefaultConstructor(const EnableDefaultConstructor&) noexcept            = default;
    constexpr EnableDefaultConstructor(EnableDefaultConstructor&&) noexcept                 = default;
    constexpr EnableDefaultConstructor& operator=(const EnableDefaultConstructor&) noexcept = default;
    constexpr EnableDefaultConstructor& operator=(EnableDefaultConstructor&&) noexcept      = default;
};

template <bool Noexcept, typename Tag>
struct EnableDefaultConstructor<true, Noexcept, Tag> {
    constexpr EnableDefaultConstructor() noexcept(Noexcept)                                 = default;
    constexpr EnableDefaultConstructor(const EnableDefaultConstructor&) noexcept            = default;
    constexpr EnableDefaultConstructor(EnableDefaultConstructor&&) noexcept                 = default;
    constexpr EnableDefaultConstructor& operator=(const EnableDefaultConstructor&) noexcept = default;
    constexpr EnableDefaultConstructor& operator=(EnableDefaultConstructor&&) noexcept      = default;
};

// EnableDestructor specialization
template <bool Noexcept, typename Tag>
struct EnableDestructor<false, Noexcept, Tag> {
    ~EnableDestructor() noexcept = delete;
};

template <bool Noexcept, typename Tag>
struct EnableDestructor<true, Noexcept, Tag> {
    ~EnableDestructor() noexcept(Noexcept) = default;
};

// EnableCopyConstructor specialization
template <bool Noexcept, typename Tag>
struct EnableCopyConstructor<false, Noexcept, Tag> {
    constexpr EnableCopyConstructor() noexcept                                        = default;
    constexpr EnableCopyConstructor(const EnableCopyConstructor&) noexcept            = delete;
    constexpr EnableCopyConstructor(EnableCopyConstructor&&) noexcept                 = default;
    constexpr EnableCopyConstructor& operator=(const EnableCopyConstructor&) noexcept = default;
    constexpr EnableCopyConstructor& operator=(EnableCopyConstructor&&) noexcept      = default;
};

template <bool Noexcept, typename Tag>
struct EnableCopyConstructor<true, Noexcept, Tag> {
    constexpr EnableCopyConstructor() noexcept                                        = default;
    constexpr EnableCopyConstructor(const EnableCopyConstructor&) noexcept(Noexcept)  = default;
    constexpr EnableCopyConstructor(EnableCopyConstructor&&) noexcept                 = default;
    constexpr EnableCopyConstructor& operator=(const EnableCopyConstructor&) noexcept = default;
    constexpr EnableCopyConstructor& operator=(EnableCopyConstructor&&) noexcept      = default;
};

// EnableCopyAssignment specialization
template <bool Noexcept, typename Tag>
struct EnableCopyAssignment<false, Noexcept, Tag> {
    constexpr EnableCopyAssignment() noexcept                                       = default;
    constexpr EnableCopyAssignment(const EnableCopyAssignment&) noexcept            = default;
    constexpr EnableCopyAssignment(EnableCopyAssignment&&) noexcept                 = default;
    constexpr EnableCopyAssignment& operator=(const EnableCopyAssignment&) noexcept = delete;
    constexpr EnableCopyAssignment& operator=(EnableCopyAssignment&&) noexcept      = default;
};

template <bool Noexcept, typename Tag>
struct EnableCopyAssignment<true, Noexcept, Tag> {
    constexpr EnableCopyAssignment() noexcept                                                 = default;
    constexpr EnableCopyAssignment(const EnableCopyAssignment&) noexcept                      = default;
    constexpr EnableCopyAssignment(EnableCopyAssignment&&) noexcept                           = default;
    constexpr EnableCopyAssignment& operator=(const EnableCopyAssignment&) noexcept(Noexcept) = default;
    constexpr EnableCopyAssignment& operator=(EnableCopyAssignment&&) noexcept                = default;
};

// EnableMoveConstructor specialization
template <bool Noexcept, typename Tag>
struct EnableMoveConstructor<false, Noexcept, Tag> {
    constexpr EnableMoveConstructor() noexcept                                        = default;
    constexpr EnableMoveConstructor(const EnableMoveConstructor&) noexcept            = default;
    constexpr EnableMoveConstructor(EnableMoveConstructor&&) noexcept                 = delete;
    constexpr EnableMoveConstructor& operator=(const EnableMoveConstructor&) noexcept = default;
    constexpr EnableMoveConstructor& operator=(EnableMoveConstructor&&) noexcept      = default;
};


template <bool Noexcept, typename Tag>
struct EnableMoveConstructor<true, Noexcept, Tag> {
    constexpr EnableMoveConstructor() noexcept                                        = default;
    constexpr EnableMoveConstructor(const EnableMoveConstructor&) noexcept            = default;
    constexpr EnableMoveConstructor(EnableMoveConstructor&&) noexcept(Noexcept)       = default;
    constexpr EnableMoveConstructor& operator=(const EnableMoveConstructor&) noexcept = default;
    constexpr EnableMoveConstructor& operator=(EnableMoveConstructor&&) noexcept      = default;
};

// EnableMoveAssignment specialization
template <bool Noexcept, typename Tag>
struct EnableMoveAssignment<false, Noexcept, Tag> {
    constexpr EnableMoveAssignment() noexcept                                       = default;
    constexpr EnableMoveAssignment(const EnableMoveAssignment&) noexcept            = default;
    constexpr EnableMoveAssignment(EnableMoveAssignment&&) noexcept                 = default;
    constexpr EnableMoveAssignment& operator=(const EnableMoveAssignment&) noexcept = default;
    constexpr EnableMoveAssignment& operator=(EnableMoveAssignment&&) noexcept      = delete;
};

template <bool Noexcept, typename Tag>
struct EnableMoveAssignment<true, Noexcept, Tag> {
    constexpr EnableMoveAssignment() noexcept                                            = default;
    constexpr EnableMoveAssignment(const EnableMoveAssignment&) noexcept                 = default;
    constexpr EnableMoveAssignment(EnableMoveAssignment&&) noexcept                      = default;
    constexpr EnableMoveAssignment& operator=(const EnableMoveAssignment&) noexcept      = default;
    constexpr EnableMoveAssignment& operator=(EnableMoveAssignment&&) noexcept(Noexcept) = default;
};
// clang-format on

} // namespace internal
} // namespace bipolar

#endif
