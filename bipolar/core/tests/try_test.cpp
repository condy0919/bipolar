#include "bipolar/core/try.hpp"

#include <gtest/gtest.h>

using namespace bipolar;

class Foo {
public:
    explicit Foo(int val) : value_(val) {}

    int value() const {
        return value_;
    }

private:
    int value_;
};

template <bool Nothrow>
class HasCtors {
public:
    explicit HasCtors(int) noexcept(Nothrow) {}
    HasCtors(HasCtors&&) noexcept(Nothrow) {}
    HasCtors& operator=(HasCtors&&) noexcept(Nothrow) {}
    HasCtors(HasCtors const&) noexcept(Nothrow) {}
    HasCtors& operator=(HasCtors const&) noexcept(Nothrow) {}
};

class MoveConstructOnly {
public:
    MoveConstructOnly() = default;
    MoveConstructOnly(const MoveConstructOnly&) = delete;
    MoveConstructOnly(MoveConstructOnly&&) = default;
};

class MutableContainer {
public:
    mutable MoveConstructOnly val;
};

TEST(Try, basic) {
    Try<int> t1(42);
    EXPECT_TRUE(t1.has_value());
    EXPECT_EQ(t1.value(), 42);

    Try<int> t2;
    EXPECT_TRUE(t2.has_nothing());

    Try<std::string> t3(std::make_exception_ptr(0xbad));
    EXPECT_THROW(t3.get(), int);
}

TEST(Try, assignmentWithThrowingMoveConstructor) {
    struct MyException : std::exception {};
    struct ThrowingMoveConstructor {
        int& counter_;
        explicit ThrowingMoveConstructor(int& counter) : counter_(counter) {
            ++counter_;
        }

        [[noreturn]] ThrowingMoveConstructor(
            ThrowingMoveConstructor&& other) noexcept(false)
            : counter_(other.counter_) {
            throw MyException{};
        }

        ThrowingMoveConstructor& operator=(ThrowingMoveConstructor&&) = delete;

        ~ThrowingMoveConstructor() {
            --counter_;
        }
    };

    int counter = 0;

    {
        Try<ThrowingMoveConstructor> t1{std::in_place, counter};
        Try<ThrowingMoveConstructor> t2{std::in_place, counter};
        EXPECT_EQ(2, counter);
        EXPECT_THROW(t2 = std::move(t1), MyException);
        EXPECT_EQ(1, counter);
        EXPECT_FALSE(t2.has_value());
        EXPECT_TRUE(t1.has_value());
    }
    EXPECT_EQ(0, counter);

    {
        Try<ThrowingMoveConstructor> t1{std::in_place, counter};
        Try<ThrowingMoveConstructor> t2;
        EXPECT_EQ(1, counter);
        EXPECT_THROW(t2 = std::move(t1), MyException);
        EXPECT_EQ(1, counter);
        EXPECT_FALSE(t2.has_value());
        EXPECT_TRUE(t1.has_value());
    }
    EXPECT_EQ(0, counter);
}

TEST(Try, emplace) {
    Try<Foo> t;

    Foo& foo = t.emplace(42);
    EXPECT_TRUE(t.has_value());
    EXPECT_EQ(foo.value(), 42);
}

TEST(Try, nothrow) {
    using F = HasCtors<false>;
    using T = HasCtors<true>;

    // default ctor
    EXPECT_TRUE(std::is_nothrow_default_constructible<Try<F>>::value);
    EXPECT_TRUE(std::is_nothrow_default_constructible<Try<T>>::value);

    // inner ctor
    EXPECT_FALSE((std::is_nothrow_constructible<Try<F>, F&&>::value));
    EXPECT_TRUE((std::is_nothrow_constructible<Try<T>, T&&>::value));
    EXPECT_FALSE((std::is_nothrow_constructible<Try<F>, F const&>::value));
    EXPECT_TRUE((std::is_nothrow_constructible<Try<T>, T const&>::value));

    // emplacing ctor
    EXPECT_FALSE(
        (std::is_nothrow_constructible<Try<F>, std::in_place_t, int>::value));
    EXPECT_TRUE(
        (std::is_nothrow_constructible<Try<T>, std::in_place_t, int>::value));

    // move ctor/assign
    EXPECT_FALSE(std::is_nothrow_move_constructible<Try<F>>::value);
    EXPECT_TRUE(std::is_nothrow_move_constructible<Try<T>>::value);
    EXPECT_FALSE(std::is_nothrow_move_assignable<Try<F>>::value);
    EXPECT_TRUE(std::is_nothrow_move_assignable<Try<T>>::value);
}

TEST(Try, MoveDereference) {
    auto ptr = std::make_unique<int>(1);
    auto t = Try<std::unique_ptr<int>>{std::move(ptr)};
    auto result = *std::move(t);
    EXPECT_EQ(*result, 1);
}

TEST(Try, MoveConstRvalue) {
    // tests to see if Try returns a const Rvalue, this is required in the case
    // where for example MutableContainer has a mutable memebr that is move only
    // and you want to fetch the value from the Try and move it into a member
    {
        const Try<MutableContainer> t{std::in_place};
        auto val = MoveConstructOnly(std::move(t).value().val);
        static_cast<void>(val);
    }
    {
        const Try<MutableContainer> t{std::in_place};
        auto val = (*(std::move(t))).val;
        static_cast<void>(val);
    }
}

TEST(Try, ValueOverloads) {
    using ML = int&;
    using MR = int&&;
    using CL = const int&;
    using CR = const int&&;

    {
        auto obj = Try<int>{};
        using ActualML = decltype(obj.value());
        using ActualMR = decltype(std::move(obj).value());
        using ActualCL = decltype(std::as_const(obj).value());
        using ActualCR = decltype(std::move(std::as_const(obj)).value());
        EXPECT_TRUE((std::is_same<ML, ActualML>::value));
        EXPECT_TRUE((std::is_same<MR, ActualMR>::value));
        EXPECT_TRUE((std::is_same<CL, ActualCL>::value));
        EXPECT_TRUE((std::is_same<CR, ActualCR>::value));
    }

    {
        auto obj = Try<int>{3};
        EXPECT_EQ(obj.value(), 3);
        EXPECT_EQ(std::move(obj).value(), 3);
        EXPECT_EQ(std::as_const(obj).value(), 3);
        EXPECT_EQ(std::move(std::as_const(obj)).value(), 3);
    }
}

// But don't choke on move-only types
TEST(Try, moveOnly) {
    Try<std::unique_ptr<int>> t;
    std::vector<Try<std::unique_ptr<int>>> v;
    v.reserve(10);
}

TEST(Try, exception) {
    using ML = std::exception_ptr&;
    using MR = std::exception_ptr&&;
    using CL = std::exception_ptr const&;
    using CR = std::exception_ptr const&&;

    {
        auto obj = Try<int>();
        using ActualML = decltype(obj.exception());
        using ActualMR = decltype(std::move(obj).exception());
        using ActualCL = decltype(std::as_const(obj).exception());
        using ActualCR = decltype(std::move(std::as_const(obj)).exception());
        EXPECT_TRUE((std::is_same<ML, ActualML>::value));
        EXPECT_TRUE((std::is_same<MR, ActualMR>::value));
        EXPECT_TRUE((std::is_same<CL, ActualCL>::value));
        EXPECT_TRUE((std::is_same<CR, ActualCR>::value));
    }

    {
        auto obj = Try<int>(3);
        EXPECT_THROW(obj.exception(), TryInvalidException);
        EXPECT_THROW(std::move(obj).exception(), TryInvalidException);
        EXPECT_THROW(std::as_const(obj).exception(), TryInvalidException);
        EXPECT_THROW(std::move(std::as_const(obj)).exception(),
                     TryInvalidException);
    }
}
