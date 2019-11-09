//! EnableSpecialMembers
//!
//! internal use only

#ifndef BIPOLAR_CORE_INTERNAL_ENABLE_SPECIAL_MEMBERS_HPP_
#define BIPOLAR_CORE_INTERNAL_ENABLE_SPECIAL_MEMBERS_HPP_

namespace bipolar {
namespace internal {

/// EnableDefaultConstructor
///
/// A mixin helper to conditionally enable/disable the default constructor
template <bool Enable, typename Tag = void>
struct EnableDefaultConstructor;

/// DisableDefaultConstructor
///
/// A mixin helper to disable the default constructor
template <typename Tag = void>
using DisableDefaultConstructor = EnableDefaultConstructor<false, Tag>;

/// EnableDestructor
///
/// A mixin helper to conditionally enable/disable the destructor
template <bool Enable, typename Tag = void>
struct EnableDestructor;

/// DisableDestructor
///
/// A mixin helper to disable the destructor
template <typename Tag = void>
using DisableDestructor = EnableDestructor<false, Tag>;

/// EnableCopy
///
/// A mixin helper to conditionally enable/disable
/// - copy constructor
/// - copy assignment
template <bool Copy, bool CopyAssignment, typename Tag = void>
struct EnableCopy;

/// DisableCopy
///
/// A mixin helper to disable
/// - copy constructor
/// - copy assignment
template <typename Tag = void>
using DisableCopy = EnableCopy<false, false, Tag>;

/// EnableMove
///
/// A mixin helper to conditionally enable/disable
/// - move constructor
/// - move assignment
template <bool Move, bool MoveAssignment, typename Tag = void>
struct EnableMove;

/// DisableMove
///
/// A mixin helper to disbale
/// - move constructor
/// - move assignment
template <typename Tag = void>
using DisableMove = EnableMove<false, false, Tag>;

/// EnableCopyMove
///
/// A mixin helper to conditionally enable/disable
/// - copy constructor
/// - move constructor
/// - copy assignment
/// - move assignment
///
/// The `Tag` template parameter is to make mixin base unique and thus avoid
/// ambiguities
template <bool Copy, bool CopyAssignment, bool Move, bool MoveAssignment,
          typename Tag = void>
struct EnableCopyMove : private EnableCopy<Copy, CopyAssignment, Tag>,
                        private EnableMove<Move, MoveAssignment, Tag> {};

/// A mixin helper to disbale
/// - copy constructor
/// - move constructor
/// - copy assignment
/// - move assignment
template <typename Tag = void>
using DisableCopyMove = EnableCopyMove<false, false, false, false, Tag>;

/// EnableSpecialMembers
///
/// A mixin helper to conditionally enable/disable the special members
///
/// The `Tag` template parameter it to make mixin base unique and thus avoid
/// ambiguities
template <bool Default, bool Destructor, bool Copy, bool CopyAssignment,
          bool Move, bool MoveAssignment, typename Tag = void>
struct EnableSpecialMembers : private EnableDefaultConstructor<Default, Tag>,
                              private EnableDestructor<Destructor, Tag>,
                              private EnableCopy<Copy, CopyAssignment, Tag>,
                              private EnableMove<Move, MoveAssignment, Tag> {};

/// DisableSpecialMembers
///
/// A mixin helper to disbale all the special members
template <typename Tag = void>
using DisableSpecialMembers =
    EnableSpecialMembers<false, false, false, false, false, false, Tag>;

// clang-format off

// Boilerplate follows

// EnableDefaultConstructor specialization
template <typename Tag>
struct EnableDefaultConstructor<false, Tag> {
    constexpr EnableDefaultConstructor() noexcept                                           = delete;
    constexpr EnableDefaultConstructor(const EnableDefaultConstructor&) noexcept            = default;
    constexpr EnableDefaultConstructor(EnableDefaultConstructor&&) noexcept                 = default;
    constexpr EnableDefaultConstructor& operator=(const EnableDefaultConstructor&) noexcept = default;
    constexpr EnableDefaultConstructor& operator=(EnableDefaultConstructor&&) noexcept      = default;
};

template <typename Tag>
struct EnableDefaultConstructor<true, Tag> {
    constexpr EnableDefaultConstructor() noexcept                                           = default;
    constexpr EnableDefaultConstructor(const EnableDefaultConstructor&) noexcept            = default;
    constexpr EnableDefaultConstructor(EnableDefaultConstructor&&) noexcept                 = default;
    constexpr EnableDefaultConstructor& operator=(const EnableDefaultConstructor&) noexcept = default;
    constexpr EnableDefaultConstructor& operator=(EnableDefaultConstructor&&) noexcept      = default;
};

// EnableDestructor specialization
template <typename Tag>
struct EnableDestructor<false, Tag> {
    ~EnableDestructor() noexcept = delete;
};

template <typename Tag>
struct EnableDestructor<true, Tag> {
    ~EnableDestructor() noexcept = default;
};

// EnableCopy specialization
template <typename Tag>
struct EnableCopy<false, false, Tag> {
    constexpr EnableCopy() noexcept                             = default;
    constexpr EnableCopy(const EnableCopy&) noexcept            = delete;
    constexpr EnableCopy(EnableCopy&&) noexcept                 = default;
    constexpr EnableCopy& operator=(const EnableCopy&) noexcept = delete;
    constexpr EnableCopy& operator=(EnableCopy&&) noexcept      = default;
};

template <typename Tag>
struct EnableCopy<true, false, Tag> {
    constexpr EnableCopy() noexcept                             = default;
    constexpr EnableCopy(const EnableCopy&) noexcept            = default;
    constexpr EnableCopy(EnableCopy&&) noexcept                 = default;
    constexpr EnableCopy& operator=(const EnableCopy&) noexcept = delete;
    constexpr EnableCopy& operator=(EnableCopy&&) noexcept      = default;
};

template <typename Tag>
struct EnableCopy<false, true, Tag> {
    constexpr EnableCopy() noexcept                             = default;
    constexpr EnableCopy(const EnableCopy&) noexcept            = delete;
    constexpr EnableCopy(EnableCopy&&) noexcept                 = default;
    constexpr EnableCopy& operator=(const EnableCopy&) noexcept = default;
    constexpr EnableCopy& operator=(EnableCopy&&) noexcept      = default;
};

template <typename Tag>
struct EnableCopy<true, true, Tag> {
    constexpr EnableCopy() noexcept                             = default;
    constexpr EnableCopy(const EnableCopy&) noexcept            = default;
    constexpr EnableCopy(EnableCopy&&) noexcept                 = default;
    constexpr EnableCopy& operator=(const EnableCopy&) noexcept = default;
    constexpr EnableCopy& operator=(EnableCopy&&) noexcept      = default;
};

// EnableMove specialization
template <typename Tag>
struct EnableMove<false, false, Tag> {
    constexpr EnableMove() noexcept                             = default;
    constexpr EnableMove(const EnableMove&) noexcept            = default;
    constexpr EnableMove(EnableMove&&) noexcept                 = delete;
    constexpr EnableMove& operator=(const EnableMove&) noexcept = default;
    constexpr EnableMove& operator=(EnableMove&&) noexcept      = delete;
};

template <typename Tag>
struct EnableMove<true, false, Tag> {
    constexpr EnableMove() noexcept                             = default;
    constexpr EnableMove(const EnableMove&) noexcept            = default;
    constexpr EnableMove(EnableMove&&) noexcept                 = default;
    constexpr EnableMove& operator=(const EnableMove&) noexcept = default;
    constexpr EnableMove& operator=(EnableMove&&) noexcept      = delete;
};

template <typename Tag>
struct EnableMove<false, true, Tag> {
    constexpr EnableMove() noexcept                             = default;
    constexpr EnableMove(const EnableMove&) noexcept            = default;
    constexpr EnableMove(EnableMove&&) noexcept                 = delete;
    constexpr EnableMove& operator=(const EnableMove&) noexcept = default;
    constexpr EnableMove& operator=(EnableMove&&) noexcept      = default;
};

template <typename Tag>
struct EnableMove<true, true, Tag> {
    constexpr EnableMove() noexcept                             = default;
    constexpr EnableMove(const EnableMove&) noexcept            = default;
    constexpr EnableMove(EnableMove&&) noexcept                 = default;
    constexpr EnableMove& operator=(const EnableMove&) noexcept = default;
    constexpr EnableMove& operator=(EnableMove&&) noexcept      = default;
};
// clang-format on

} // namespace internal
} // namespace bipolar

#endif
