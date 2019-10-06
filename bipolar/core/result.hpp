/// \file result.hpp
/// \see Result

#ifndef BIPOLAR_CORE_RESULT_HPP_
#define BIPOLAR_CORE_RESULT_HPP_

#include <new>
#include <utility>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

#include "bipolar/core/void.hpp"
#include "bipolar/core/traits.hpp"
#include "bipolar/core/option.hpp"


/// \brief Macro sugar for unwraping a \c Result. Early return when \c Err state
///
/// \param expr of Result\<T, E\> return type
#define BIPOLAR_TRY(expr)                                                      \
    ({                                                                         \
        static_assert(::bipolar::detail::is_result_v<decltype((expr))>);       \
        auto result = (expr);                                                  \
        if (!bool(result)) {                                                   \
            return result;                                                     \
        }                                                                      \
        ::std::move(result.value());                                           \
    })

namespace bipolar {
template <typename T, typename E>
class Result;

/// \brief Ok variant of Result
/// \see Result
template <typename T>
struct Ok {
    constexpr Ok(T&& val) : value(std::move(val)) {}

    constexpr Ok(const T& val) : value(val) {}

    template <typename E>
    constexpr Result<T, E> into() && {
        return {std::move(*this)};
    }

    T value;
};

/// \brief Err variant of Result
/// \see Result
template <typename E>
struct Err {
    constexpr Err(E&& val) : value(std::move(val)) {}

    constexpr Err(const E& val) : value(val) {}

    template <typename T>
    constexpr Result<T, E> into() && {
        return {std::move(*this)};
    }

    E value;
};


namespace detail {
/// @{
/// \internal
/// \class is_result
/// \brief Checks whether \c T is Result or not
/// \endinternal
template <typename T>
struct is_result_impl : std::false_type {};

template <typename T, typename E>
struct is_result_impl<Result<T, E>> : std::true_type {};

template <typename T>
using is_result = is_result_impl<std::decay_t<T>>;

template <typename T>
inline constexpr bool is_result_v = is_result<T>::value;
/// @}


/// @{
/// \internal
/// \name all_of
/// \brief Checks whether all \c T meet the \c Trait or not
/// \endinternal
template <template <typename> typename Trait, typename... Ts>
using all_of = std::conjunction<Trait<Ts>...>;

template <template <typename> typename Trait, typename... Ts>
inline constexpr bool all_of_v = all_of<Trait, Ts...>::value;
/// @}


/// \internal
/// \brief The data type that storage holds
/// \endinternal
enum class Which : unsigned char {
    Empty, ///< Only used in construction
    Value, ///< Ok
    Error, ///< Err
};

struct EmptyTag {};
struct ValueTag {};
struct ErrorTag {};

/// @{
/// \internal
/// \struct StorageTrait
/// \brief With \c CRTP idiom, \c Derived class is still incomplete.
/// Using traits as workaround.
///
/// \see https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
/// \endinternal
template <typename>
struct StorageTrait;

template <typename, typename>
struct SmallTrivialStorage;

template <typename, typename>
struct TrivialStorage;

template <typename, typename>
struct NonTrivialStorage;

template <typename T, typename E>
struct StorageTrait<SmallTrivialStorage<T, E>> {
    using value_type = T;
    using error_type = E;
};

template <typename T, typename E>
struct StorageTrait<TrivialStorage<T, E>> {
    using value_type = T;
    using error_type = E;
};

template <typename T, typename E>
struct StorageTrait<NonTrivialStorage<T, E>> {
    using value_type = T;
    using error_type = E;
};
/// @}


/// \internal
/// \brief The storage interface type using \c CRTP
/// \related StorageTrait
/// \endinternal
template <typename Derived>
class Storage {
    using T = typename StorageTrait<Derived>::value_type;
    using E = typename StorageTrait<Derived>::error_type;

public:
    /// \internal
    /// \brief Assigns a value
    /// - For \c trivial types, uses assignment operator
    /// - For \c nontrivial types, it will be overridden
    /// \related SmallTrivialStorage
    /// \related TrivialStorage
    /// \related NonTrivialStorage
    /// \endinternal
    template <typename... Ts>
    constexpr void assign_value(Ts&&... ts) {
        that()->clear();
        that()->which(Which::Value);
        // consider T is const qualified
        if constexpr (std::is_assignable_v<T, T>) {
            that()->value_ = T(std::forward<Ts>(ts)...);
        } else {
            new ((void*)std::addressof(value())) T(std::forward<Ts>(ts)...);
        }
    }

    /// \internal
    /// \brief Assigns an error
    /// - For \c trivial types, uses assignment operator
    /// - For \c nontrivial types, it will be overridden
    /// \related SmallTrivialStorage
    /// \related TrivialStorage
    /// \related NonTrivialStorage
    /// \endinternal
    template <typename... Es>
    constexpr void assign_error(Es&&... es) {
        that()->clear();
        that()->which(Which::Error);
        // consider T is const qualified
        if constexpr (std::is_assignable_v<E, E>) {
            that()->error_ = E(std::forward<Es>(es)...);
        } else {
            new ((void*)std::addressof(error())) E(std::forward<Es>(es)...);
        }
    }

    /// \internal
    /// \brief Assigns with the others
    /// \endinternal
    template <typename U>
    constexpr void assign(U&& rhs) {
        switch (rhs.which()) {
        case Which::Value:
            that()->assign_value(std::forward<U>(rhs).value());
            break;

        case Which::Error:
            that()->assign_error(std::forward<U>(rhs).error());
            break;

        default:
            break;
        }
    }

    /// \internal
    /// \brief Resets to empty
    /// - For \c trivial types, it does nothing
    /// - For \c nontrivial types, it will be overridden
    /// \endinternal
    constexpr void clear() {}

    /// @{
    /// \internal
    /// \brief Returns the \c value
    /// \endinternal
    constexpr T& value() & {
        return that()->value_;
    }

    constexpr const T& value() const& {
        return that()->value_;
    }

    constexpr T&& value() && {
        return std::move(that()->value_);
    }

    constexpr const T&& value() const&& {
        return std::move(that()->value_);
    }
    /// @}


    /// @{
    /// \internal
    /// \brief Returns the \c error
    /// \endinternal
    constexpr E& error() & {
        return that()->error_;
    }

    constexpr const E& error() const& {
        return that()->error_;
    }

    constexpr E&& error() && {
        return std::move(that()->error_);
    }

    constexpr const E&& error() const&& {
        return std::move(that()->error_);
    }
    /// @}

    /// @{
    /// \internal
    /// \brief Returns the holding data type
    /// \endinternal
    constexpr Which which() const {
        return that()->which_;
    }

    /// \brief Sets holding data type
    constexpr void which(Which w) {
        that()->which_ = w;
    }
    /// @}

private:
    constexpr Derived* that() {
        return static_cast<Derived*>(this);
    }

    constexpr const Derived* that() const {
        return static_cast<const Derived*>(this);
    }
};

/// \internal
/// \struct SmallTrivialStorage
/// \brief Small trivial storage type
/// For small (pointer-sized) trivial types, a struct is faster than a union
/// Benchmark on `Intel(R) Core(TM) i5-8250U CPU @ 1.60GHz`
///
/// | Storage |    CPU (ns) |
/// |---------+-------------|
/// | struct  | 7.42        |
/// | union   | 7.81        |
/// \endinternal
template <typename T, typename E>
struct SmallTrivialStorage : Storage<SmallTrivialStorage<T, E>> {
    static_assert(std::is_trivial_v<T>, "T must be trivial");
    static_assert(std::is_trivial_v<E>, "E must be trivial");
    static_assert(sizeof(std::pair<T, E>) <= sizeof(void* [2]),
                  "SmallTrivialStorage isn't smaller than 2 pointers");

    Which which_;
    T value_;
    E error_;

    explicit constexpr SmallTrivialStorage(EmptyTag) noexcept(
        std::is_nothrow_default_constructible_v<T>&&
            std::is_nothrow_default_constructible_v<E>)
        : which_(Which::Empty), value_{}, error_{} {}

    template <typename... Ts>
    explicit constexpr SmallTrivialStorage(ValueTag, Ts&&... ts) noexcept(
        std::is_nothrow_constructible_v<T, Ts...>&&
            std::is_nothrow_default_constructible_v<E>)
        : which_(Which::Value), value_(std::forward<Ts>(ts)...), error_{} {}

    template <typename... Es>
    explicit constexpr SmallTrivialStorage(ErrorTag, Es&&... es) noexcept(
        std::is_nothrow_constructible_v<E, Es...>&&
            std::is_nothrow_default_constructible_v<T>)
        : which_(Which::Error), value_{}, error_(std::forward<Es>(es)...) {}
};

/// \internal
/// \struct TrivialStorage
/// \brief Trivial storage type
/// No need to destructs value/error because it's trivial.
/// \endinternal
template <typename T, typename E>
struct TrivialStorage : Storage<TrivialStorage<T, E>> {
    static_assert(std::is_trivially_copyable_v<T>,
                  "T must be trivially copyable");
    static_assert(std::is_trivially_copyable_v<E>,
                  "E must be trivially copyable");

    Which which_;
    union {
        T value_;
        E error_;
        char dummy_;
    };

    explicit constexpr TrivialStorage(EmptyTag) noexcept
        : which_(Which::Empty), dummy_('\0') {}

    template <typename... Ts>
    explicit constexpr TrivialStorage(ValueTag, Ts&&... ts) noexcept(
        std::is_nothrow_constructible_v<T, Ts...>)
        : which_(Which::Value), value_(std::forward<Ts>(ts)...) {}

    template <typename... Es>
    explicit constexpr TrivialStorage(ErrorTag, Es&&... es) noexcept(
        std::is_nothrow_constructible_v<E, Es...>)
        : which_(Which::Error), error_(std::forward<Es>(es)...) {}
};

// clang-format off
#define BIPOLAR_TRAIT_ENABLE_GEN(t,                                            \
                                 copy_ctor_noexcept_with_body,                 \
                                 move_ctor_noexcept_with_body,                 \
                                 copy_assign_noexcept_with_body,               \
                                 move_assign_noexcept_with_body)               \
    template <typename Derived, bool, bool Noexcept>                           \
    struct t {                                                                 \
        constexpr t() noexcept = default;                                      \
        constexpr t(const t& rhs) copy_ctor_noexcept_with_body                 \
        constexpr t(t&& rhs) move_ctor_noexcept_with_body                      \
        constexpr t& operator=(const t& rhs) copy_assign_noexcept_with_body    \
        constexpr t& operator=(t&& rhs) move_assign_noexcept_with_body         \
    };

#define BIPOLAR_TRAIT_DISABLE_GEN(t,                                           \
                                  copy_ctor_noexcept_with_body,                \
                                  move_ctor_noexcept_with_body,                \
                                  copy_assign_noexcept_with_body,              \
                                  move_assign_noexcept_with_body)              \
    template <typename Derived, bool Noexcept>                                 \
    struct t<Derived, false, Noexcept> {                                       \
        constexpr t() noexcept = default;                                      \
        constexpr t(const t&) copy_ctor_noexcept_with_body                     \
        constexpr t(t&&) move_ctor_noexcept_with_body                          \
        constexpr t& operator=(const t&) copy_assign_noexcept_with_body        \
        constexpr t& operator=(t&&) move_assign_noexcept_with_body             \
    };

BIPOLAR_TRAIT_ENABLE_GEN(
    CopyConstructible,
    noexcept(Noexcept) {
        static_cast<Derived*>(this)->assign(static_cast<const Derived&>(rhs));
    },
    = default;,
    = default;,
    = default;)
BIPOLAR_TRAIT_DISABLE_GEN(
    CopyConstructible,
    = delete;,
    = default;,
    = default;,
    = default;)

BIPOLAR_TRAIT_ENABLE_GEN(
    MoveConstructible,
    = default;,
    noexcept(Noexcept) {
        static_cast<Derived*>(this)->assign(std::move(static_cast<Derived&>(rhs)));
    },
    = default;,
    = default;)
BIPOLAR_TRAIT_DISABLE_GEN(
    MoveConstructible,
    = default;,
    = delete;,
    = default;,
    = default;)

BIPOLAR_TRAIT_ENABLE_GEN(
    CopyAssignable,
    = default;,
    = default;,
    noexcept(Noexcept) {
        static_cast<Derived*>(this)->assign(static_cast<const Derived&>(rhs));
        return *this;
    },
    = default;)
BIPOLAR_TRAIT_DISABLE_GEN(
    CopyAssignable,
    = default;,
    = default;,
    = delete;,
    = default;)

BIPOLAR_TRAIT_ENABLE_GEN(
    MoveAssignable,
    = default;,
    = default;,
    = default;,
    noexcept(Noexcept) {
        static_cast<Derived*>(this)->assign(std::move(static_cast<Derived&>(rhs)));
        return *this;
    })
BIPOLAR_TRAIT_DISABLE_GEN(
    MoveAssignable,
    = default;,
    = default;,
    = default;,
    = delete;)
// clang-format on

#undef BIPOLAR_TRAIT_ENABLE_GEN
#undef BIPOLAR_TRAIT_DISABLE_GEN


/// \internal
/// \brief The inner storage type used with non-trivial types.
/// \see NonTrivialStorage
/// \endinternal
template <typename T, typename E>
struct NonTrivialStorageInner {
    Which which_ = Which::Empty;
    union {
        T value_;
        E error_;
        char dummy_;
    };

    explicit constexpr NonTrivialStorageInner(EmptyTag) noexcept
        : which_(Which::Empty), dummy_('\0') {}

    template <typename... Ts>
    explicit constexpr NonTrivialStorageInner(ValueTag, Ts&&... ts) noexcept(
        std::is_nothrow_constructible_v<T, Ts...>)
        : which_(Which::Value), value_(std::forward<Ts>(ts)...) {}

    template <typename... Es>
    explicit constexpr NonTrivialStorageInner(ErrorTag, Es&&... es) noexcept(
        std::is_nothrow_constructible_v<E, Es...>)
        : which_(Which::Error), error_(std::forward<Es>(es)...) {}

    constexpr NonTrivialStorageInner(const NonTrivialStorageInner&) {}

    constexpr NonTrivialStorageInner(NonTrivialStorageInner&&) noexcept {}

    constexpr NonTrivialStorageInner& operator=(const NonTrivialStorageInner&) {
        return *this;
    }

    constexpr NonTrivialStorageInner&
    operator=(NonTrivialStorageInner&&) noexcept {
        return *this;
    }

    ~NonTrivialStorageInner() {}
};

/// \internal
/// \struct NonTrivialStorage
/// \brief nontrivial storage type
/// - \e copy-ctor requires \c T and \c E all copy constructible
/// - \e move-ctor requires \c T and \c E all move constructible
/// - \e copy-assign requires \e copy-ctor and \c T and \c E all copy assignable
/// - \e move-assign requires \e move-ctor and \c T and \c E all move assignable
/// \endinternal
template <typename T, typename E>
struct NonTrivialStorage
    : Storage<NonTrivialStorage<T, E>>,
      NonTrivialStorageInner<T, E>,
      CopyConstructible<NonTrivialStorage<T, E>,
                        all_of_v<std::is_copy_constructible, T, E>,
                        all_of_v<std::is_nothrow_copy_constructible, T, E>>,
      MoveConstructible<NonTrivialStorage<T, E>,
                        all_of_v<std::is_move_constructible, T, E>,
                        all_of_v<std::is_nothrow_move_constructible, T, E>>,
      CopyAssignable<NonTrivialStorage<T, E>,
                     all_of_v<std::is_copy_constructible, T, E> &&
                         all_of_v<std::is_copy_assignable, T, E>,
                     all_of_v<std::is_nothrow_copy_constructible, T, E> &&
                         all_of_v<std::is_nothrow_copy_assignable, T, E>>,
      MoveAssignable<NonTrivialStorage<T, E>,
                     all_of_v<std::is_move_constructible, T, E> &&
                         all_of_v<std::is_move_assignable, T, E>,
                     all_of_v<std::is_nothrow_move_constructible, T, E> &&
                         all_of_v<std::is_nothrow_move_assignable, T, E>> {

    using Base = Storage<NonTrivialStorage<T, E>>;

    using NonTrivialStorageInner<T, E>::NonTrivialStorageInner;

    /// @{
    /// \internal
    /// \brief Makes it look like a trivial type like \c SmallTrivialStorage
    /// and \c TrivialStorage
    /// \endinternal
    constexpr NonTrivialStorage(const NonTrivialStorage&) = default;
    constexpr NonTrivialStorage(NonTrivialStorage&&) = default;
    /// @}

    /// @{
    /// \internal
    /// \brief Makes it look like a trivial type like \c SmallTrivialStorage
    /// and \c TrivialStorage
    /// \endinternal
    constexpr NonTrivialStorage& operator=(const NonTrivialStorage&) = default;
    constexpr NonTrivialStorage& operator=(NonTrivialStorage&&) = default;
    /// @}

    ~NonTrivialStorage() {
        clear();
    }

    /// \internal
    /// \brief Destructs current value/error
    /// Overrides (actually shadows) the \c Base empty clear
    /// \endinternal
    constexpr void clear() /* override */ {
        switch (this->which_) {
        case Which::Value:
            this->value_.~T();
            break;

        case Which::Error:
            this->error_.~E();
            break;

        default:
            break;
        }
    }

    template <typename... Ts>
    constexpr void assign_value(Ts&&... ts) {
        this->clear();
        this->which(Which::Value);
        new ((void*)std::addressof(this->value())) T(std::forward<Ts>(ts)...);
    }

    template <typename... Es>
    constexpr void assign_error(Es&&... es) {
        this->clear();
        this->which(Which::Error);
        new ((void*)std::addressof(this->value())) E(std::forward<Es>(es)...);
    }

    template <typename U>
    constexpr void assign(U&& rhs) {
        if constexpr (std::is_same_v<std::decay_t<U>, NonTrivialStorage>) {
            if (this == std::addressof(rhs)) {
                return;
            }
        }
        this->Base::assign(std::forward<U>(rhs));
    }
};

/// \internal
/// The storage type which \c Result will use
/// \endinternal
template <typename T, typename E>
using ResultStorage = std::conditional_t<
    all_of_v<std::is_trivial, T, E> && sizeof(std::pair<T, E>) <= sizeof(void* [2]),
    SmallTrivialStorage<T, E>,
    std::conditional_t<all_of_v<std::is_trivially_copyable, T, E>,
                       TrivialStorage<T, E>, NonTrivialStorage<T, E>>>;
} // namespace detail


/// \class BadResultAccess
/// \brief Throws on logic errors
class BadResultAccess : public std::logic_error {
public:
    BadResultAccess() : std::logic_error("Bad result access") {}

    BadResultAccess(const char* s) : std::logic_error(s) {}
};


/// \class Result
/// \brief Result is a type that represents either success Ok or failure
/// \tparam T The success type
/// \tparam E The error type
///
/// \c Result is the type used for returning and propagating errors. It's a
/// \c std::pair like class with the variants, \c Ok, representing success and
/// containing a value, and \c Err, representing error and containing an error.
/// Functions return \c Result whenever errors are expected and recoverable.
///
/// A simple function returning \c Result might be defined and used like so:
///
/// ```
/// Result<int, const char*> parse(const char* s) {
///     if (std::strlen(s) < 3) {
///         return Err("string length is less than 3");
///     }
///     return Ok(s[0] * 100 + s[1] * 10 + s[2]);
/// }
/// ```
///
/// \c Result has similar combinators with \c Option, see document comments for
/// details.
template <typename T, typename E>
class Result final : public detail::ResultStorage<T, E> {
    static_assert(!std::is_reference_v<T>,
                  "Result cannot be used with reference type");
    static_assert(!std::is_abstract_v<T>,
                  "Result cannot be used with abstract type");

    using Base = detail::ResultStorage<T, E>;

public:
    using value_type = T;
    using error_type = E;

    ////////////////////////////////////////////////////////////////////////////
    // Constructors & Assignments
    ////////////////////////////////////////////////////////////////////////////

    /// @{
    /// \brief Constructs from \c Ok variant
    ///
    /// ```
    /// const Result<int, int> x(Ok(42));
    /// assert(x.has_value());
    /// ```
    template <bool C = true,
              std::enable_if_t<C && std::is_move_constructible_v<T>, int> = 0>
    constexpr Result(Ok<T>&& ok) noexcept(
        std::is_nothrow_move_constructible_v<T>)
        : Base{detail::ValueTag{}, std::move(ok.value)} {}

    template <bool C = true,
              std::enable_if_t<C && std::is_copy_constructible_v<T>, int> = 0>
    constexpr Result(const Ok<T>& ok) noexcept(
        std::is_nothrow_copy_constructible_v<T>)
        : Base{detail::ValueTag{}, ok.value} {}
    /// @}


    /// @{
    /// \brief Constructs from \c Err variant
    ///
    /// ```
    /// const Result<int, int> x(Err(42));
    /// assert(x.has_error());
    /// ```
    template <bool C = true,
              std::enable_if_t<C && std::is_move_constructible_v<E>, int> = 0>
    constexpr Result(Err<E>&& err) noexcept(
        std::is_nothrow_move_constructible_v<E>)
        : Base{detail::ErrorTag{}, std::move(err.value)} {}

    template <bool C = true,
              std::enable_if_t<C && std::is_copy_constructible_v<E>, int> = 0>
    constexpr Result(const Err<E>& err) noexcept(
        std::is_nothrow_copy_constructible_v<E>)
        : Base{detail::ErrorTag{}, err.value} {}
    /// @}


    /// @{
    /// \brief Constructs from other type `Result<X, Y>` where
    /// \c T can be constructible with \c X and
    /// \c E can be constructible with \c Y
    ///
    /// \tparam X \c T can be constructed from \c X
    /// \tparam Y \c E can be constructed from \c Y
    template <typename X, typename Y,
              std::enable_if_t<!std::is_same_v<Result<X, Y>, Result<T, E>> &&
                                   std::is_constructible_v<T, X> &&
                                   std::is_constructible_v<E, Y>,
                               int> = 0>
    constexpr Result(const Result<X, Y>& rhs) noexcept(
        std::is_nothrow_constructible_v<T, X>&&
            std::is_nothrow_constructible_v<E, Y>)
        : Base{detail::EmptyTag{}} {
        Base::assign(rhs);
    }

    template <typename X, typename Y,
              std::enable_if_t<!std::is_same_v<Result<X, Y>, Result<T, E>> &&
                                   std::is_constructible_v<T, X&&> &&
                                   std::is_constructible_v<E, Y&&>,
                               int> = 0>
    constexpr Result(Result<X, Y>&& rhs) noexcept(
        std::is_nothrow_constructible_v<T, X&&>&&
            std::is_nothrow_constructible_v<E, Y&&>)
        : Base{detail::EmptyTag{}} {
        Base::assign(std::move(rhs));
    }
    /// @}


    /// @{
    /// \brief Default constructs from other results
    /// Disabled if \c T or \c E is not \e move-constructible or \e copy-constructible
    constexpr Result(Result&&) = default;
    constexpr Result(const Result&) = default;
    /// @}


    /// @{
    /// \brief Assigns with others
    /// Disabled if \c T or \c E is not \e move-constructible or \e copy-constructible
    constexpr Result& operator=(Result&&) = default;
    constexpr Result& operator=(const Result&) = default;
    /// @}
    

    /// @{
    /// \see assign
    constexpr Result& operator=(const Ok<T>& ok) {
        Base::assign_value(ok.value);
        return *this;
    }

    constexpr Result& operator=(Ok<T>&& ok) {
        Base::assign_value(std::move(ok.value));
        return *this;
    }
    /// @}


    /// @{
    /// \see assign
    constexpr Result& operator=(const Err<E>& err) {
        Base::assign_error(err.value);
        return *this;
    }

    constexpr Result& operator=(Err<E>&& err) {
        Base::assign_error(std::move(err.value));
        return *this;
    }
    /// @}


    /// @{
    /// \brief Assigns with \c Ok type
    /// Exports \c assign_value interface
    constexpr void assign(const Ok<T>& ok) {
        Base::assign_value(ok.value);
    }

    constexpr void assign(Ok<T>&& ok) {
        Base::assign_value(std::move(ok.value));
    }
    /// @}

    /// @{
    /// \brief Assigns with \c Err type
    /// Exports \c assign_error interface
    constexpr void assign(const Err<E>& err) {
        Base::assign_error(err.value);
    }

    constexpr void assign(Err<E>&& err) {
        Base::assign_error(std::move(err.value));
    }
    /// @}


    ////////////////////////////////////////////////////////////////////////////
    // Combinators
    ////////////////////////////////////////////////////////////////////////////

    /// @{
    /// \brief Maps a Result<T, E> to Result<U, E> by applying a function
    /// to the contained \c Ok value, leaving an \c Err value untouched.
    ///
    /// \tparam F :: T -> U
    /// \return Result\<U, E\>
    ///
    /// ```
    /// const Result<int, int> x(Ok(2));
    /// const auto res = x.map([](int x) { return x + 1; });
    /// assert(res.has_value() && res.value() == 3);
    /// ```
    template <typename F, typename U = std::invoke_result_t<F, T>>
    constexpr Result<U, E> map(F&& f) const& {
        return has_value() ? Ok(std::forward<F>(f)(value()))
                           : static_cast<Result<U, E>>(Err(error()));
    }

    template <typename F, typename U = std::invoke_result_t<F, T>>
    constexpr Result<U, E> map(F&& f) && {
        return has_value() ? Ok(std::forward<F>(f)(std::move(value())))
                           : static_cast<Result<U, E>>(Err(std::move(error())));
    }
    /// @}


    /// @{
    /// \brief Maps a Result<T, E> to U by applying a function to the contained
    /// \c Ok value, or a fallback function to a contained \c Err value.
    ///
    /// \tparam F :: T -> U
    /// \tparam M :: E -> U
    /// \return U
    ///
    /// ```
    /// const int k = 21;
    ///
    /// const Result<int, int> x(Ok(2));
    /// const auto res = x.map_or_else([k]() { return 2 * k; },
    ///                                [](int x) { return x; });
    /// assert(res == 2);
    ///
    /// const Result<int, int> y(Err(2));
    /// const auto res2 = y.map_or_else([k]() { return 2 * k; },
    ///                                 [](int x) { return x; });
    /// assert(res2 == 42);
    /// ```
    template <typename M, typename F, typename U = std::invoke_result_t<F, T>,
              std::enable_if_t<std::is_same_v<U, std::invoke_result_t<M, E>>,
                               int> = 0>
    constexpr U map_or_else(M&& fallback, F&& f) const& {
        return has_value() ? std::forward<F>(f)(value())
                           : std::forward<M>(fallback)(error());
    }

    template <typename M, typename F, typename U = std::invoke_result_t<F, T>,
              std::enable_if_t<std::is_same_v<U, std::invoke_result_t<M, E>>,
                               int> = 0>
    constexpr U map_or_else(M&& fallback, F&& f) && {
        return has_value() ? std::forward<F>(f)(std::move(value()))
                           : std::forward<M>(fallback)(std::move(error()));
    }
    /// @}


    /// @{
    /// \brief Maps a `Result<T, E>` to `Result<T, U>` by applying a function
    /// to the contained \c Err value, leaving an \c Ok value untouched.
    ///
    /// \tparam F :: E -> U
    /// \return Result\<T, U\>
    ///
    /// ```
    /// const Result<int, int> x(Ok(2));
    /// const auto res = x.map_err([](int x) { return x + 1; });
    /// assert(!res.has_error());
    ///
    /// const Result<int, int> y(Err(3));
    /// const auto res2 = y.map_err([](int x) { return x + 1; });
    /// assert(res2.has_error() && res2.error() == 4);
    /// ```
    template <typename F, typename U = std::invoke_result_t<F, E>>
    constexpr Result<T, U> map_err(F&& f) const& {
        return has_value() ? static_cast<Result<T, U>>(Ok(value()))
                           : Err(std::forward<F>(f)(error()));
    }

    template <typename F, typename U = std::invoke_result_t<F, E>>
    constexpr Result<T, U> map_err(F&& f) && {
        return has_value() ? static_cast<Result<T, U>>(Ok(std::move(value())))
                           : Err(std::forward<F>(f)(std::move(error())));
    }
    /// @}


    /// @{
    /// \brief Calls \c f if the result is \c Ok, otherwise returns the \c Err
    /// value of self.
    ///
    /// \tparam F :: T -> U
    /// \return U \c Result type
    ///
    /// ```
    /// Result<int, int> sq(int x) {
    ///     return Ok(x * x);
    /// }
    ///
    /// Result<int, int> err(int x) {
    ///     return Err(x);
    /// }
    ///
    /// const Result<int, int> x(Ok(2));
    /// assert(x.and_then(sq).and_then(sq) == Ok(16));
    /// assert(x.and_then(sq).and_then(err) == Err(4));
    /// assert(x.and_then(err).and_then(sq) == Err(2));
    /// assert(x.and_then(err).and_then(err) == Err(2));
    /// ```
    template <typename F, typename U = std::invoke_result_t<F, T>,
              std::enable_if_t<detail::is_result_v<U> &&
                                   std::is_same_v<typename U::error_type, E>,
                               int> = 0>
    constexpr U and_then(F&& f) const& {
        return has_value() ? std::forward<F>(f)(value())
                           : static_cast<U>(Err(error()));
    }

    template <typename F, typename U = std::invoke_result_t<F, T>,
              std::enable_if_t<detail::is_result_v<U> &&
                                   std::is_same_v<typename U::error_type, E>,
                               int> = 0>
    constexpr U and_then(F&& f) && {
        return has_value() ? std::forward<F>(f)(std::move(value()))
                           : static_cast<U>(Err(std::move(error())));
    }
    /// @}


    /// @{
    /// \brief Calls \c f if the result is \c Err, otherwise returns the
    /// \c Ok value of self.
    ///
    /// \tparam F :: E -> U
    /// \return U \c Result type
    ///
    /// ```
    /// Result<int, int> sq(int x) {
    ///     return Ok(x * x);
    /// }
    ///
    /// Result<int, int> err(int x) {
    ///     return Err(x);
    /// }
    ///
    /// const Result<int, int> x(Ok(2)), y(Err(3));
    /// assert(x.or_else(sq).or_else(sq) == Ok(2));
    /// assert(x.or_else(err).or_else(sq) == Ok(2));
    /// assert(y.or_else(sq).or_else(err) == Ok(9));
    /// assert(y.or_else(err).or_else(err) == Err(3));
    /// ```
    template <typename F, typename U = std::invoke_result_t<F, E>,
              std::enable_if_t<detail::is_result_v<U> &&
                                   std::is_same_v<typename U::value_type, T>,
                               int> = 0>
    constexpr U or_else(F&& f) const& {
        return has_value() ? static_cast<U>(Ok(value()))
                           : std::forward<F>(f)(error());
    }

    template <typename F, typename U = std::invoke_result_t<F, E>,
              std::enable_if_t<detail::is_result_v<U> &&
                                   std::is_same_v<typename U::value_type, T>,
                               int> = 0>
    constexpr U or_else(F&& f) && {
        return has_value() ? static_cast<U>(Ok(std::move(value())))
                           : std::forward<F>(f)(std::move(error()));
    }
    /// @}


    ///////////////////////////////////////////////////////////////////////////
    // Utilities
    ///////////////////////////////////////////////////////////////////////////

    /// \brief Returns \c true if the result is an \c Ok value containing the
    /// given value.
    ///
    /// ```
    /// const Result<int, int> x(Ok(2));
    /// assert(x.contains(2));
    /// assert(!x.contains(3));
    ///
    /// const Result<int, int> y(Err(2));
    /// assert(!y.contains(2));
    /// ```
    template <typename U,
              std::enable_if_t<is_equality_comparable_v<T, U>, int> = 0>
    constexpr bool contains(const U& x) const noexcept {
        return has_value() ? Base::value() == x : false;
    }


    /// \brief Returns \c true if the result is an \c Err value containing the
    /// given value.
    ///
    /// ```
    /// const Result<int, int> x(Ok(2));
    /// assert(!x.contains_err(2));
    ///
    /// const Result<int, int> y(Err(2));
    /// assert(y.contains_err(2));
    ///
    /// const Result<int, int> z(Err(3));
    /// assert(!z.contains_err(2));
    /// ```
    template <typename U,
              std::enable_if_t<is_equality_comparable_v<E, U>, int> = 0>
    constexpr bool contains_err(const U& x) const noexcept {
        return has_error() ? Base::error() == x : false;
    }


    /// @{
    /// \brief Converts from `Result<T, E>` to `Option<T>`
    /// If it's an error, discards the error value.
    ///
    /// \return Option\<T\>
    ///
    /// ```
    /// const Result<int, int> x(Ok(2));
    /// const auto optx = x.ok();
    /// assert(optx.has_value() && optx.value() == 2);
    ///
    /// const Result<int, int> y(Err(2));
    /// const auto opty = y.ok();
    /// assert(!opty.has_value());
    /// ```
    constexpr Option<T> ok() const& {
        return has_value() ? Some(value()) : None;
    }

    constexpr Option<T> ok() && {
        return has_value() ? Some(std::move(value())) : None;
    }
    /// @}


    /// @{
    /// \brief Converts from `Result<T, E>` to `Option<E>`
    /// If it's an ok, discards the ok value.
    ///
    /// \return Option\<E\>
    ///
    /// ```
    /// const Result<int, int> x(Ok(2));
    /// const auto optx = x.err();
    /// assert(!optx.has_value());
    ///
    /// const Result<int, int> y(Err(2));
    /// const auto opty = y.err();
    /// assert(opty.has_value() && opty.value() == 2);
    /// ```
    constexpr Option<E> err() const& {
        return has_error() ? Some(error()) : None;
    }

    constexpr Option<E> err() && {
        return has_error() ? Some(std::move(error())) : None;
    }
    /// @}


    /// \brief Inplacement constructs from args
    /// \return T&
    ///
    /// ```
    /// Result<std::string, int> x(Err(2));
    /// assert(x.has_error() && x.error() == 2);
    /// 
    /// x.emplace("foo");
    /// assert(x.has_value() && x.value() == "foo");
    /// ```
    template <typename... Args>
    constexpr T& emplace(Args&&... args) &
        noexcept(std::is_nothrow_constructible_v<T, Args...>) {
        Base::assign_value(std::forward<Args>(args)...);
        return Base::value();
    }

    /// \brief Returns \c true if the result is \c Ok variant
    ///
    /// ```
    /// Result<int, int> res = Ok(3);
    /// assert(res.has_value());
    ///
    /// Result<int, int> err_res = Err(3);
    /// assert(!res.has_value());
    /// ```
    constexpr bool has_value() const noexcept {
        return Base::which() == detail::Which::Value;
    }

    /// \brief Returns \c true if the result is \c Err variant
    ///
    /// ```
    /// Result<int, int> err_res = Err(3);
    /// assert(err_res.has_error());
    /// ```
    constexpr bool has_error() const noexcept {
        return Base::which() == detail::Which::Error;
    }


    /// @{
    /// \brief Unwraps a result, yielding the content of an \c Ok.
    /// Else, it returns \c deft
    ///
    /// Arguments passed to \c value_or are eagerly evaluated; if you are
    /// passing the result of a function call, it is recommended to use
    /// \c value_or_else, which is lazily evaluated.
    ///
    /// \return T
    /// \see value_or_else
    ///
    /// ```
    /// const Result<int, int> x(Ok(2));
    /// assert(x.value_or(3) == 2);
    ///
    /// const Result<int, int> y(Err(2));
    /// assert(y.value_or(3) == 3);
    /// ```
    template <typename U>
    constexpr T value_or(U&& deft) const& {
        return has_value() ? value()
                           : static_cast<T>(std::forward<U>(deft));
    }

    template <typename U>
    constexpr T value_or(U&& deft) && {
        return has_value() ? std::move(value())
                           : static_cast<T>(std::forward<U>(deft));
    }
    /// @}


    /// @{
    /// \brief Unwraps a result, yielding the content of an \c Ok
    /// If the value is an \c Err, then it calls \c f with its \c Err
    ///
    /// \tparam F :: E -> T
    /// \return T
    ///
    /// ```
    /// const Result<int, int> x(Ok(2));
    /// const int resx = x.value_or_else([](int x) { return x; });
    /// assert(resx == 2);
    ///
    /// const Result<int, int> y(Err(3));
    /// const int resy = y.value_or_else([](int y) { return y; });
    /// assert(resy == 3);
    /// ```
    template <typename F,
              std::enable_if_t<std::is_same_v<std::invoke_result_t<F, E>, T>,
                               int> = 0>
    constexpr T value_or_else(F&& f) const& {
        return has_value() ? value()
                           : std::forward<F>(f)(error());
    }

    template <typename F,
              std::enable_if_t<std::is_same_v<std::invoke_result_t<F, E>, T>,
                               int> = 0>
    constexpr T value_or_else(F&& f) && {
        return has_value() ? std::move(value())
                           : std::forward<F>(f)(std::move(error()));
    }
    /// @}


    /// @{
    /// \brief Unwraps a result, yielding the content of an \c Ok
    /// \throw BadResultAccess
    constexpr T& value() & {
        value_required();
        return Base::value();
    }

    constexpr const T& value() const& {
        value_required();
        return Base::value();
    }

    constexpr T&& value() && {
        value_required();
        return std::move(Base::value());
    }

    constexpr const T&& value() const&& {
        value_required();
        return std::move(Base::value());
    }
    /// @}


    /// @{
    /// \brief Unwraps a result, yielding the content of an \c Ok
    /// \param s exception description
    /// \throw BadResultAccess
    constexpr const T& expect(const char* s) const& {
        value_required(s);
        return Base::value();
    }

    constexpr const T&& expect(const char* s) const&& {
        value_required(s);
        return Base::value();
    }
    /// @}


    /// @{
    /// \brief Unwraps a result, yielding the content of an \c Err
    /// \throw BadResultAccess
    constexpr E& error() & {
        error_required();
        return Base::error();
    }

    constexpr const E& error() const& {
        error_required();
        return Base::error();
    }

    constexpr E&& error() && {
        error_required();
        return std::move(Base::error());
    }

    constexpr const E&& error() const&& {
        error_required();
        return std::move(Base::error());
    }
    /// @}


    /// @{
    /// \brief Unwraps a result, yielding the content of an \c Err
    /// \param s exception description
    /// \throw BadResultAccess
    constexpr const E& expect_err(const char* s) const& {
        error_required(s);
        return Base::error();
    }

    constexpr const E&& expect_err(const char* s) && {
        error_required(s);
        return std::move(Base::error());
    }
    /// @}

    /// \brief Checks whether the result is \c Ok or not
    /// \return \c true => Ok\n
    ///         \c false => Err
    /// \see has_value
    explicit constexpr operator bool() const noexcept {
        return has_value();
    }

    /// @{
    /// \brief Dereference makes it more like a pointer
    /// \throw BadResultAccess
    constexpr T& operator*() & {
        return value();
    }

    constexpr const T& operator*() const& {
        return value();
    }

    constexpr T&& operator*() && {
        return std::move(value());
    }

    constexpr const T&& operator*() const&& {
        return std::move(value());
    }
    /// @}

    /// @{
    /// \brief Arrow operator makes it more like a pointer
    /// \throw BadResultAccess
    constexpr const T* operator->() const {
        return std::addressof(value());
    }

    constexpr T* operator->() {
        return std::addressof(value());
    }
    /// @}


    /// @{
    /// \brief Returns the pointer to internal storage
    /// If it's an \c Err, \c nullptr is returned.
    ///
    /// \return T* or \c nullptr
    constexpr T* get_pointer() & noexcept {
        return has_value() ? std::addressof(Base::value()) : nullptr;
    }

    constexpr const T* get_pointer() const& noexcept {
        return has_value() ? std::addressof(Base::value()) : nullptr;
    }

    constexpr T* get_pointer() && noexcept = delete;
    /// @}


    /// \brief Swaps with other results
    /// \note \c Base::value and \c Base::error are noexcept
    constexpr void swap(Result& rhs) noexcept(
        detail::all_of_v<std::is_nothrow_swappable, T, E>) {
        using std::swap;
        if (has_value()) {
            if (rhs.has_value()) {
                swap(Base::value(), rhs.Base::value());
            } else {
                E err = std::move(rhs.Base::error());
                rhs.Base::assign_value(std::move(Base::value()));
                Base::assign_error(std::move(err));
            }
        } else {
            if (rhs.has_error()) {
                swap(Base::error(), rhs.Base::error());
            } else {
                T ok = std::move(rhs.Base::value());
                rhs.Base::assign_error(std::move(Base::error()));
                Base::assign_value(std::move(ok));
            }
        }
    }

private:
    constexpr void value_required() const {
        if (!has_value()) {
            throw BadResultAccess();
        }
    }

    constexpr void value_required(const char* s) const {
        if (!has_value()) {
            throw BadResultAccess(s);
        }
    }

    constexpr void error_required() const {
        if (!has_error()) {
            throw BadResultAccess();
        }
    }

    constexpr void error_required(const char* s) const {
        if (!has_error()) {
            throw BadResultAccess(s);
        }
    }
};


/// \brief Swaps Results
/// \see Result\<T, E\>::swap
template <typename T, typename E>
constexpr void swap(Result<T, E>& lhs, Result<T, E>& rhs) noexcept(
    detail::all_of_v<std::is_nothrow_swappable, T, E>) {
    lhs.swap(rhs);
}


/// @{
/// \brief Comparison with others
template <
    typename T, typename E,
    std::enable_if_t<is_equality_comparable_v<T> && is_equality_comparable_v<E>,
                     int> = 0>
inline constexpr bool operator==(const Result<T, E>& lhs,
                                 const Result<T, E>& rhs) {
    if (lhs.has_value() && rhs.has_value()) {
        return lhs.value() == rhs.value();
    } else if (lhs.has_error() == rhs.has_error()) {
        return lhs.error() == rhs.error();
    }
    return false;
}

template <
    typename T, typename E,
    std::enable_if_t<is_equality_comparable_v<T> && is_equality_comparable_v<E>,
                     int> = 0>
inline constexpr bool operator!=(const Result<T, E>& lhs,
                                 const Result<T, E>& rhs) {
    return !(lhs == rhs);
}

template <
    typename T, typename E,
    std::enable_if_t<
        is_less_than_comparable_v<T> && is_less_than_comparable_v<E>, int> = 0>
inline constexpr bool operator<(const Result<T, E>& lhs,
                                const Result<T, E>& rhs) {
    if (lhs.has_value()) {
        if (rhs.has_value()) {
            return lhs.value() < rhs.value();
        } else {
            return false;
        }
    } else {
        if (rhs.has_error()) {
            return lhs.error() < rhs.error();
        } else {
            return true;
        }
    }
}

template <
    typename T, typename E,
    std::enable_if_t<
        is_less_than_comparable_v<T> && is_less_than_comparable_v<E>, int> = 0>
inline constexpr bool operator<=(const Result<T, E>& lhs,
                                 const Result<T, E>& rhs) {
    return !(rhs < lhs);
}

template <
    typename T, typename E,
    std::enable_if_t<
        is_less_than_comparable_v<T> && is_less_than_comparable_v<E>, int> = 0>
inline constexpr bool operator>(const Result<T, E>& lhs,
                                const Result<T, E>& rhs) {
    return rhs < lhs;
}

template <
    typename T, typename E,
    std::enable_if_t<
        is_less_than_comparable_v<T> && is_less_than_comparable_v<E>, int> = 0>
inline constexpr bool operator>=(const Result<T, E>& lhs,
                                 const Result<T, E>& rhs) {
    return !(lhs < rhs);
}
/// @}


/// @{
/// \brief Specialized comparisons with \c Ok Type
/// \c Ok is in the right
template <typename T, typename E,
          std::enable_if_t<is_equality_comparable_v<T>, int> = 0>
inline constexpr bool operator==(const Result<T, E>& lhs, const Ok<T>& rhs) {
    return lhs.has_value() && lhs.value() == rhs.value;
}

template <typename T, typename E,
          std::enable_if_t<is_equality_comparable_v<T>, int> = 0>
inline constexpr bool operator!=(const Result<T, E>& lhs, const Ok<T>& rhs) {
    return !(lhs == rhs);
}

template <typename T, typename E,
          std::enable_if_t<is_less_than_comparable_v<T>, int> = 0>
inline constexpr bool operator<(const Result<T, E>& lhs, const Ok<T>& rhs) {
    return lhs.has_error() || lhs.value() < rhs.value;
}

template <typename T, typename E,
          std::enable_if_t<is_less_than_comparable_v<T>, int> = 0>
inline constexpr bool operator<=(const Result<T, E>& lhs, const Ok<T>& rhs) {
    return lhs.has_error() || lhs.value() <= rhs.value;
}

template <typename T, typename E,
          std::enable_if_t<is_less_than_comparable_v<T>, int> = 0>
inline constexpr bool operator>(const Result<T, E>& lhs, const Ok<T>& rhs) {
    return lhs.has_value() && lhs.value() > rhs.value;
}

template <typename T, typename E,
          std::enable_if_t<is_less_than_comparable_v<T>, int> = 0>
inline constexpr bool operator>=(const Result<T, E>& lhs, const Ok<T>& rhs) {
    return lhs.has_value() && lhs.value() >= rhs.value;
}
/// @}

/// @{
/// \brief Specialized comparisons with \c Ok Type
/// \c Ok is in the left
template <typename T, typename E,
          std::enable_if_t<is_equality_comparable_v<T>, int> = 0>
inline constexpr bool operator==(const Ok<T>& lhs, const Result<T, E>& rhs) {
    return rhs == lhs;
}

template <typename T, typename E,
          std::enable_if_t<is_equality_comparable_v<T>, int> = 0>
inline constexpr bool operator!=(const Ok<T>& lhs, const Result<T, E>& rhs) {
    return rhs != lhs;
}

template <typename T, typename E,
          std::enable_if_t<is_less_than_comparable_v<T>, int> = 0>
inline constexpr bool operator<(const Ok<T>& lhs, const Result<T, E>& rhs) {
    return rhs > lhs;
}

template <typename T, typename E,
          std::enable_if_t<is_less_than_comparable_v<T>, int> = 0>
inline constexpr bool operator<=(const Ok<T>& lhs, const Result<T, E>& rhs) {
    return rhs >= lhs;
}

template <typename T, typename E,
          std::enable_if_t<is_less_than_comparable_v<T>, int> = 0>
inline constexpr bool operator>(const Ok<T>& lhs, const Result<T, E>& rhs) {
    return rhs < lhs;
}

template <typename T, typename E,
          std::enable_if_t<is_less_than_comparable_v<T>, int> = 0>
inline constexpr bool operator>=(const Ok<T>& lhs, const Result<T, E>& rhs) {
    return rhs <= lhs;
}
/// @}


/// @{
/// \brief Specialized comparisons with \c Err Type
/// \c Err is in the right
template <typename T, typename E,
          std::enable_if_t<is_equality_comparable_v<E>, int> = 0>
inline constexpr bool operator==(const Result<T, E>& lhs, const Err<E>& rhs) {
    return lhs.has_error() && lhs.error() == rhs.value;
}

template <typename T, typename E,
          std::enable_if_t<is_equality_comparable_v<E>, int> = 0>
inline constexpr bool operator!=(const Result<T, E>& lhs, const Err<E>& rhs) {
    return !(lhs == rhs);
}

template <typename T, typename E,
          std::enable_if_t<is_less_than_comparable_v<E>, int> = 0>
inline constexpr bool operator<(const Result<T, E>& lhs, const Err<E>& rhs) {
    return lhs.has_error() && lhs.error() < rhs.value;
}

template <typename T, typename E,
          std::enable_if_t<is_less_than_comparable_v<E>, int> = 0>
inline constexpr bool operator<=(const Result<T, E>& lhs, const Err<E>& rhs) {
    return lhs.has_error() && lhs.error() <= rhs.value;
}

template <typename T, typename E,
          std::enable_if_t<is_less_than_comparable_v<E>, int> = 0>
inline constexpr bool operator>(const Result<T, E>& lhs, const Err<E>& rhs) {
    return lhs.has_value() || lhs.error() > rhs.value;
}

template <typename T, typename E,
          std::enable_if_t<is_less_than_comparable_v<E>, int> = 0>
inline constexpr bool operator>=(const Result<T, E>& lhs, const Err<E>& rhs) {
    return lhs.has_value() || lhs.error() >= rhs.value;
}
/// @}

/// @{
/// \brief Specialized comparisons with \c Err Type
/// \c Err is in the left
template <typename T, typename E,
          std::enable_if_t<is_equality_comparable_v<E>, int> = 0>
inline constexpr bool operator==(const Err<E>& lhs, const Result<T, E>& rhs) {
    return rhs == lhs;
}

template <typename T, typename E,
          std::enable_if_t<is_equality_comparable_v<E>, int> = 0>
inline constexpr bool operator!=(const Err<E>& lhs, const Result<T, E>& rhs) {
    return rhs != lhs;
}

template <typename T, typename E,
          std::enable_if_t<is_less_than_comparable_v<E>, int> = 0>
inline constexpr bool operator<(const Err<E>& lhs, const Result<T, E>& rhs) {
    return rhs > lhs;
}

template <typename T, typename E,
          std::enable_if_t<is_less_than_comparable_v<E>, int> = 0>
inline constexpr bool operator<=(const Err<E>& lhs, const Result<T, E>& rhs) {
    return rhs >= lhs;
}

template <typename T, typename E,
          std::enable_if_t<is_less_than_comparable_v<E>, int> = 0>
inline constexpr bool operator>(const Err<E>& lhs, const Result<T, E>& rhs) {
    return rhs < lhs;
}

template <typename T, typename E,
          std::enable_if_t<is_less_than_comparable_v<E>, int> = 0>
inline constexpr bool operator>=(const Err<E>& lhs, const Result<T, E>& rhs) {
    return rhs <= lhs;
}
/// @}

} // namespace bipolar

#endif
