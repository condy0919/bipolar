//! Function
//!
//! see `Function` type for details
//!

#ifndef BIPOLAR_CORE_FUNCTION_HPP_
#define BIPOLAR_CORE_FUNCTION_HPP_

#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

namespace bipolar {
template <typename>
class Function;

/// Function
///
/// # Brief
///
/// `Function` is a general-purpose polymorphic function wrapper.
/// Instances of `Function` can store, move, and invoke any callable targets.
///
/// `Function` is similar with `std::function` without `CopyConstructible` and
/// `CopyAssignable` requirements of callable targets. As a result, `Function`
/// is not copyable.
///
/// The stored callable object is called the target of `Function`.
/// If a `Function` contains no target, it is called empty. Invoking the target
/// of an empty `Function` results in `std::bad_function_call` exception being
/// thrown.
///
/// NOTE:
/// If your construct a `Function` with a `NULL` function pointer, bool(*this)
/// still equal to true. That's an another difference with `std::function`
///
/// # Examples
///
/// Normal uses:
///
/// ```
/// // lambda
/// int x = 1000;
/// Function<int(int)> f([&x](int v) { return x += v; });
/// assert(1005 == f(5));
/// assert(1011 == f(6));
///
/// // member function
/// Function<std::size_t(const std::string&)>
///     size_member_func(std::mem_fn(&std::string::size));
/// assert(5 == size_member_func("12345"s));
/// ```
///
/// Some bad uses:
///
/// ```
/// Function<int()> empty_func;
/// empty_func(); // std::bad_function_call thrown
///
/// Function<int()> empty_func2(nullptr);
/// empty_func2(); // std::bad_function_call thrown
///
/// using FuncType = int (*)();
/// Function<int()> empty_func3(FuncType(0));
/// empty_func3(); // std::bad_function_call thrown
/// ```
template <typename R, typename... Args>
class Function<R(Args...)> {
    using InsituType = void* [8];

    template <typename F>
    inline static constexpr bool
        Insitu = sizeof(F) <= sizeof(InsituType) &&
                 alignof(F) <= alignof(InsituType) &&
                 std::is_nothrow_move_constructible_v<F>;

    struct Vtable {
        void (*destroy)(const Function* pf);
        R (*call)(const Function* pf, Args...);
    };
    
    static void empty_destroy_(const Function*) {}
    static R empty_call_(const Function*, Args...) {
        throw std::bad_function_call();
    }

    // A constexpr specifier used in static member variable (since C++17)
    // declaration implies inline.
    //
    // See https://en.cppreference.com/w/cpp/language/constexpr
    static constexpr Vtable empty_vtable_ = {
        empty_destroy_,
        empty_call_,
    };

public:
    /// Creates an empty `Function` call wrapper
    ///
    /// ```
    /// Function<int()> empty1;
    /// assert(bool(empty1) == false);
    ///
    /// Function<int()> empty2(nullptr);
    /// assert(bool(empty2) == false);
    /// ```
    constexpr Function() noexcept : vtbl_(&empty_vtable_), avail_(false), stg_() {}
    constexpr Function(std::nullptr_t) noexcept : Function() {}

    /// `Function` is move-only
    Function(const Function&) = delete;
    Function& operator=(const Function&) = delete;

    /// Creates a `Function` that targets the functor
    ///
    /// ```
    /// // lambda
    /// Function<int()> f1([](int x) { return ++x; });
    ///
    /// // pointer to function
    /// using FuncPtr = int (*)();
    /// Function<int()> f2(FuncPtr([](int x) { return ++x; }));
    ///
    /// // UB: NULL pointer to function
    /// Function<int()> f3(FuncType(0));
    ///
    /// // pointer to member function, use `std::mem_fn` please
    /// Function<std::size_t(const std::string&)>
    ///         f4(std::mem_fn(&std::string::size));
    /// ```
    template <
        typename F,
        std::enable_if_t<
            std::is_invocable_r_v<R, F, Args...> &&
                !std::is_same_v<std::remove_cv_t<std::remove_reference_t<F>>,
                                Function>,
            int> = 0>
    explicit Function(F&& f) noexcept(Insitu<F>)
        : Function(std::forward<F>(f), std::bool_constant<Insitu<F>>{}) {}

    /// `Function` move constructor
    Function(Function&& rhs) noexcept : Function() {
        rhs.swap(*this);
    }

    /// `Function` assignment to zero
    Function& operator=(std::nullptr_t) noexcept {
        Function().swap(*this);
        return *this;
    }

    /// `Function` assignment with a Functor
    ///
    /// see `Function::Function` for details
    template <
        typename F,
        std::enable_if_t<
            std::is_invocable_r_v<R, F, Args...> &&
                !std::is_same_v<std::remove_cv_t<std::remove_reference_t<F>>,
                                Function>,
            int> = 0>
    Function& operator=(F&& f) noexcept(Insitu<F>) {
        Function(std::forward<F>(f)).swap(*this);
        return *this;
    }

    /// `Function` move assignment
    ///
    /// see `Function::Function` for details
    Function& operator=(Function&& rhs) noexcept {
        Function(std::move(rhs)).swap(*this);
        return *this;
    }

    ~Function() {
        vtbl_->destroy(this);
    }

    /// Invokes the function targeted
    /// Throws a `std::bad_function_call` exception if `!bool(*this)`
    R operator()(Args... args) const {
        return vtbl_->call(this, std::forward<Args>(args)...);
    }

    /// Determines if the `Function` wrapper has a target
    explicit operator bool() const noexcept {
        return avail_;
    }

    /// Swaps the targets of two `Function` objects
    void swap(Function& rhs) noexcept {
        std::swap(vtbl_, rhs.vtbl_);
        std::swap(avail_, rhs.avail_);
        std::swap(stg_, rhs.stg_);
    }

private:
    // insitu case
    template <typename F>
    explicit Function(F&& f, std::true_type) noexcept {
        using NoRefF = std::remove_reference_t<F>;

        struct Op {
            static NoRefF* access(const Function* pf) {
                return static_cast<NoRefF*>(const_cast<void*>(
                    static_cast<const void*>(&pf->stg_.insitu_)));
            }

            static std::remove_reference_t<F>* access(Function* pf) {
                return static_cast<NoRefF*>(
                    static_cast<void*>(&pf->stg_.insitu_));
            }

            static void init(Function* pf, F&& f) {
                new (const_cast<void*>(static_cast<const void*>(access(pf))))
                    NoRefF(std::forward<F>(f));
            }

            static void destroy(const Function* pf) {
                (void)pf;
                if constexpr (!std::is_trivially_destructible_v<F>) {
                    access(pf)->~F();
                }
            }

            static R call(const Function* pf, Args... args) {
                return (*access(pf))(std::forward<Args>(args)...);
            }
        };

        static constexpr Vtable vtable = {
            Op::destroy,
            Op::call,
        };

        vtbl_ = &vtable;
        avail_ = true;
        Op::init(this, std::forward<F>(f));
    }

    // heap case
    template <typename F>
    explicit Function(F&& f, std::false_type) {
        using NoRefF = std::remove_reference_t<F>;

        struct Op {
            static NoRefF* access(const Function* pf) {
                return static_cast<NoRefF*>(const_cast<void*>(pf->stg_.ptr_));
            }

            static void init(Function* pf, F&& f) {
                pf->stg_.ptr_ = new NoRefF(std::forward<F>(f));
            }

            static void destroy(const Function* pf) {
                delete access(pf);
            }

            static R call(const Function* pf, Args... args) {
                return (*access(pf))(std::forward<Args>(args)...);
            }
        };

        static constexpr Vtable vtable = {
            Op::destroy,
            Op::call,
        };

        vtbl_ = &vtable;
        avail_ = true;
        Op::init(this, std::forward<F>(f));
    }

private:
    const Vtable* vtbl_;
    bool avail_;
    union {
        void* ptr_;
        std::aligned_union_t<0, InsituType> insitu_;
    } stg_;
};

/// Swaps the targets of two polymorphic function object wrappers
template <typename T>
inline void swap(Function<T>& lhs, Function<T>& rhs) noexcept {
    lhs.swap(rhs);
}

/// Compares a polymorphic function object wrapper against `nullptr`
template <typename T>
inline bool operator==(const Function<T>& lhs, std::nullptr_t) noexcept {
    return !static_cast<bool>(lhs);
}

template <typename T>
inline bool operator==(std::nullptr_t, const Function<T>& lhs) noexcept {
    return !static_cast<bool>(lhs);
}

template <typename T>
inline bool operator!=(const Function<T>& lhs, std::nullptr_t) noexcept {
    return static_cast<bool>(lhs);
}

template <typename T>
inline bool operator!=(std::nullptr_t, const Function<T>& lhs) noexcept {
    return static_cast<bool>(lhs);
}

} // namespace bipolar

#endif
