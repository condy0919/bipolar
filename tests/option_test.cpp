#include "bipolar/option.hpp"
#include <cstdint>
#include <string>
#include <memory>
#include <ostream>
#include <unordered_map>
#include <gtest/gtest.h>

using namespace bipolar;
using namespace std::literals;

template <typename T>
std::ostream& operator<<(std::ostream& os, const Option<T>& opt) {
    if (opt.has_value()) {
        return os << "Some(" << opt.value() << ")";
    } else {
        return os << "None";
    }
}

TEST(Option, ConstexprConstructible) {
    constexpr Option<int> opt(None);
    EXPECT_FALSE(opt.has_value());
}

TEST(Option, Reset) {
    Option<int> opt(3);
    opt.clear();
    EXPECT_FALSE(opt.has_value());
}

TEST(Option, String) {
    Option<std::string> maybe_string(None);
    EXPECT_FALSE(maybe_string.has_value());

    maybe_string.assign("hello");
    EXPECT_TRUE(maybe_string.has_value());
}

TEST(Option, Const) {
    // copy-ctor
    const int x = 6;
    Option<const int> opt(x);
    EXPECT_EQ(opt.value(), 6);

    // move-ctor
    const int y = 7;
    Option<const int> opt2(std::move(y));
    EXPECT_EQ(opt2.value(), 7);
}

TEST(Option, Simple) {
    Option<int> opt(None);

    EXPECT_FALSE(opt.has_value());
    EXPECT_EQ(42, opt.value_or(42));

    opt = 4;
    EXPECT_TRUE(opt.has_value());
    EXPECT_EQ(4, opt.value());
    EXPECT_EQ(4, opt.value_or(42));

    opt = 5;
    EXPECT_EQ(5, opt.value());

    opt.clear();
    EXPECT_FALSE(opt.has_value());

    EXPECT_FALSE(bool(opt));
    EXPECT_EQ(42, opt.value_or_else([]() { return 42; }));
}

class MoveTester {
public:
    MoveTester(const char* s) : s_(s) {}
    MoveTester(const MoveTester&) = default;
    MoveTester(MoveTester&& other) noexcept {
        s_ = std::move(other.s_);
        other.s_ = "";
    }

    MoveTester& operator=(const MoveTester&) = default;
    MoveTester& operator=(MoveTester&& other) noexcept {
        s_ = std::move(other.s_);
        other.s_ = "";
        return *this;
    }

    std::string& get() {
        return s_;
    }

    friend bool operator==(const MoveTester& lhs, const MoveTester& rhs) {
        return lhs.s_ == rhs.s_;
    }

private:
    std::string s_;
};

TEST(Option, value_or_rvalue_arg) {
    Option<MoveTester> opt(None);
    MoveTester dflt = "hello";
    EXPECT_EQ("hello", opt.value_or(dflt));
    EXPECT_EQ("hello", dflt);
    EXPECT_EQ("hello", opt.value_or(std::move(dflt)));
    EXPECT_EQ("", dflt);
    EXPECT_EQ("world", opt.value_or("world"));

    dflt = "hello";
    // Make sure that the const overload works on const objects
    const auto& optc = opt;
    EXPECT_EQ("hello", optc.value_or(dflt));
    EXPECT_EQ("hello", dflt);
    EXPECT_EQ("hello", optc.value_or(std::move(dflt)));
    EXPECT_EQ("", dflt);
    EXPECT_EQ("world", optc.value_or("world"));

    dflt = "hello";
    opt.assign("meow");
    EXPECT_EQ("meow", opt.value_or(dflt));
    EXPECT_EQ("hello", dflt);
    EXPECT_EQ("meow", opt.value_or(std::move(dflt)));
    EXPECT_EQ("hello", dflt); // only moved if used

    opt.emplace("meow");
    const auto result =
        std::move(opt).and_then([](MoveTester mt) -> Option<std::string> {
            return Some(std::move(mt.get()));
        });
    EXPECT_EQ("", opt.value());
    EXPECT_EQ("meow", result.value());
}

TEST(Option, value_or_noncopyable) {
    Option<std::unique_ptr<int>> opt(None);
    std::unique_ptr<int> dflt(new int(42));
    EXPECT_EQ(42, *std::move(opt).value_or(std::move(dflt)));
}

struct ExpectingDeleter {
    int expected;

    explicit ExpectingDeleter(int expected_) : expected(expected_) {}

    void operator()(const int* ptr) {
        EXPECT_EQ(*ptr, expected);
        delete ptr;
    }
};

TEST(Option, value_move) {
    auto ptr = Option<std::unique_ptr<int, ExpectingDeleter>>(
            {new int(42), ExpectingDeleter{1337}})
    .value();
    *ptr = 1337;
}

TEST(Option, dereference_move) {
    auto ptr = *Option<std::unique_ptr<int, ExpectingDeleter>>(
            {new int(42), ExpectingDeleter{1337}});
    *ptr = 1337;
}

TEST(Option, EmptyConstruct) {
    Option<int> opt;
    EXPECT_FALSE(bool(opt));

    Option<int> test1(opt);
    EXPECT_FALSE(bool(test1));

    Option<int> test2(std::move(opt));
    EXPECT_FALSE(bool(test2));
}

TEST(Option, Unique) {
    Option<std::unique_ptr<int>> opt;

    opt.clear();
    EXPECT_FALSE(bool(opt));

    // empty->emplaced
    opt.emplace(new int(5));
    EXPECT_TRUE(bool(opt));
    EXPECT_EQ(5, **opt);

    opt.clear();
    // empty->moved
    opt = std::make_unique<int>(6);
    EXPECT_EQ(6, **opt);

    // full->moved
    opt = std::make_unique<int>(7);
    EXPECT_EQ(7, **opt);

    // move it out by move construct
    Option<std::unique_ptr<int>> moved(std::move(opt));
    EXPECT_TRUE(bool(moved));
    EXPECT_FALSE(bool(opt));
    EXPECT_EQ(7, **moved);

    EXPECT_TRUE(bool(moved));
    opt = std::move(moved); // move it back by move assign
    EXPECT_FALSE(bool(moved));
    EXPECT_TRUE(bool(opt));
    EXPECT_EQ(7, **opt);
}

TEST(Option, Shared) {
    std::shared_ptr<int> ptr;
    Option<std::shared_ptr<int>> opt;
    EXPECT_FALSE(bool(opt));

    // empty->emplaced
    opt.emplace(new int(5));
    EXPECT_TRUE(bool(opt));
    ptr = opt.value();
    EXPECT_EQ(ptr.get(), opt->get());
    EXPECT_EQ(2, ptr.use_count());
    opt.clear();
    EXPECT_EQ(1, ptr.use_count());

    // full->copied
    opt = ptr;
    EXPECT_EQ(2, ptr.use_count());
    EXPECT_EQ(ptr.get(), opt->get());
    opt.clear();
    EXPECT_EQ(1, ptr.use_count());

    // full->moved
    opt = std::move(ptr);
    EXPECT_EQ(1, opt->use_count());
    EXPECT_EQ(nullptr, ptr.get());
    {
        Option<std::shared_ptr<int>> copied(opt);
        EXPECT_EQ(2, opt->use_count());

        Option<std::shared_ptr<int>> moved(std::move(opt));
        EXPECT_EQ(2, moved->use_count());

        moved.emplace(new int(6));
        EXPECT_EQ(1, moved->use_count());
        copied = moved;
        EXPECT_EQ(2, moved->use_count());
    }
}

TEST(Option, Order) {
    std::vector<Option<int>> vect{
        {None},
        {3},
        {1},
        {None},
        {2},
    };
    std::vector<Option<int>> expected{
        {None},
        {None},
        {1},
        {2},
        {3},
    };
    std::sort(vect.begin(), vect.end());
    EXPECT_EQ(vect, expected);
}

TEST(Option, Swap) {
    Option<std::string> a;
    Option<std::string> b;

    swap(a, b);
    EXPECT_FALSE(a.has_value());
    EXPECT_FALSE(b.has_value());

    a.assign("hello");
    EXPECT_TRUE(a.has_value());
    EXPECT_FALSE(b.has_value());
    EXPECT_EQ("hello", a.value());

    swap(a, b);
    EXPECT_FALSE(a.has_value());
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ("hello", b.value());

    a.assign("bye");
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ("bye", a.value());

    swap(a, b);
    EXPECT_TRUE(a.has_value());
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ("hello", a.value());
    EXPECT_EQ("bye", b.value());
}

TEST(Option, Comparisons) {
    Option<int> o_;
    Option<int> o1(1);
    Option<int> o2(2);

    EXPECT_TRUE(o_ <= (o_));
    EXPECT_TRUE(o_ == (o_));
    EXPECT_TRUE(o_ >= (o_));

    EXPECT_TRUE(o1 < o2);
    EXPECT_TRUE(o1 <= o2);
    EXPECT_TRUE(o1 <= (o1));
    EXPECT_TRUE(o1 == (o1));
    EXPECT_TRUE(o1 != o2);
    EXPECT_TRUE(o1 >= (o1));
    EXPECT_TRUE(o2 >= o1);
    EXPECT_TRUE(o2 > o1);

    EXPECT_FALSE(o2 < o1);
    EXPECT_FALSE(o2 <= o1);
    EXPECT_FALSE(o2 <= o1);
    EXPECT_FALSE(o2 == o1);
    EXPECT_FALSE(o1 != (o1));
    EXPECT_FALSE(o1 >= o2);
    EXPECT_FALSE(o1 >= o2);
    EXPECT_FALSE(o1 > o2);
}

TEST(Option, Conversions) {
    Option<bool> mbool;
    Option<short> mshort;
    Option<char*> mstr;
    Option<int> mint;

    // These don't compile
    // bool b = mbool;
    // short s = mshort;
    // char* c = mstr;
    // int x = mint;
    // char* c(mstr);
    // short s(mshort);
    // int x(mint);

    // intended explicit operator bool, for if (opt).
    bool b(mbool);
    EXPECT_FALSE(b);

    // Truthy tests work and are not ambiguous
    if (mbool && mshort && mstr && mint) { // only checks not-empty
        if (*mbool && *mshort && *mstr && *mint) { // only checks value
            ;
        }
    }

    mbool = false;
    EXPECT_TRUE(bool(mbool));
    EXPECT_FALSE(*mbool);

    mbool = true;
    EXPECT_TRUE(bool(mbool));
    EXPECT_TRUE(*mbool);

    mbool = None;
    EXPECT_FALSE(bool(mbool));

    // No conversion allowed; does not compile
    // EXPECT_TRUE(mbool == false);
}

TEST(Option, map) {
    Option<std::string> empty(None);

    EXPECT_FALSE(bool(empty.map(std::mem_fn(&std::string::size))));

    const auto result = Some("hello"s).map(std::mem_fn(&std::string::size));
    EXPECT_TRUE(bool(result));
    EXPECT_EQ(5, result.value());
}

TEST(Option, map_or) {
    Option<std::string> empty(None);

    EXPECT_EQ(0, empty.map_or(0, std::mem_fn(&std::string::size)));

    EXPECT_EQ(5, Some("hello"s).map_or(0, std::mem_fn(&std::string::size)));
}

TEST(Option, map_or_else) {
    Option<std::string> empty(None);

    EXPECT_EQ(42, empty.map_or_else([]() -> std::size_t { return 42; },
                                   std::mem_fn(&std::string::size)));
    EXPECT_EQ(5, Some("hello"s).map_or_else([]() -> std::size_t { return 42; },
                                            std::mem_fn(&std::string::size)));
}

TEST(Option, and_then) {
    Option<std::string> empty(None);

    const auto result =
        empty.and_then([](const std::string& s) -> Option<std::size_t> {
            return Some(s.size());
        });
    EXPECT_FALSE(bool(result));

    const auto result2 = Some("hello"s).and_then(
        [](std::string s) -> Option<std::size_t> { return Some(s.size()); });
    EXPECT_EQ(result2.value(), 5);
}

TEST(Option, filter) {
    Option<std::string> opt("hello"s);

    const auto result =
        opt.filter([](const std::string& s) { return s.size() == 3; });
    EXPECT_FALSE(bool(result));

    const auto result2 = opt.take().filter(
        [](const std::string& s) { return s.compare(0, 3, "hel", 0, 3) == 0; });
    EXPECT_FALSE(bool(opt));
    EXPECT_TRUE(bool(result2));
    EXPECT_EQ("hello", result2.value());
}

TEST(Option, or_else) {
    Option<std::string> opt("hello"s);

    const auto result =
        opt.or_else([]() -> Option<std::string> { return Some("world"s); });
    EXPECT_EQ("hello", opt.value());

    const auto result2 = opt.take();
    EXPECT_FALSE(bool(opt));
    EXPECT_TRUE(bool(result2));

    const auto result3 =
        opt.or_else([]() -> Option<std::string> { return Some("world"s); });
    EXPECT_EQ(result3.value(), "world");
}

TEST(Option, take) {
    Option<std::string> opt("hello"s);

    auto other = opt.take();
    EXPECT_FALSE(bool(opt));
    EXPECT_EQ("hello", other.value());

    auto other2 = opt.take();
    EXPECT_FALSE(bool(other2));
}

TEST(Option, Pointee) {
    Option<int> x;
    EXPECT_FALSE(x.get_pointer());
    x = 1;
    EXPECT_TRUE(x.get_pointer());
    *x.get_pointer() = 2;
    EXPECT_TRUE(*x == 2);
    x = None;
    EXPECT_FALSE(x.get_pointer());
}

TEST(Option, SelfAssignment) {
    Option<int> a = 42;
    a = static_cast<decltype(a)&>(a); // suppress self-assign warning
    ASSERT_TRUE(a.has_value() && a.value() == 42);

    Option<int> b = 23333333;
    b = static_cast<decltype(b)&&>(b); // suppress self-move warning
    ASSERT_TRUE(b.has_value() && b.value() == 23333333);
}


class ContainsOption {
public:
    ContainsOption() {}

    explicit ContainsOption(int x) : opt_(x) {}

    bool has_value() const {
        return opt_.has_value();
    }

    int value() const {
        return opt_.value();
    }

    ContainsOption(const ContainsOption& other) = default;
    ContainsOption& operator=(const ContainsOption& other) = default;
    ContainsOption(ContainsOption&& other) = default;
    ContainsOption& operator=(ContainsOption&& other) = default;

private:
    Option<int> opt_;
};

/**
 * Test that a class containing an Option can be copy and move assigned.
 * This was broken under gcc 4.7 until assignment operators were explicitly
 * defined.
 */
TEST(Option, AssignmentContained) {
    {
        ContainsOption source(5), target;
        target = source;
        EXPECT_TRUE(target.has_value());
        EXPECT_EQ(5, target.value());
    }

    {
        ContainsOption source(5), target;
        target = std::move(source);
        EXPECT_TRUE(target.has_value());
        EXPECT_EQ(5, target.value());
        EXPECT_FALSE(source.has_value());
    }

    {
        ContainsOption opt_uninit, target(10);
        target = opt_uninit;
        EXPECT_FALSE(target.has_value());
    }
}

TEST(Option, Exceptions) {
    Option<int> empty;
    EXPECT_THROW(empty.value(), OptionEmptyException);
}

TEST(Option, NoThrowDefaultConstructible) {
    EXPECT_TRUE(std::is_nothrow_default_constructible<Option<bool>>::value);
}

struct NoDestructor {};

struct WithDestructor {
    ~WithDestructor();
};

TEST(Option, TriviallyDestructible) {
    // These could all be static_asserts but EXPECT_* give much nicer output on
    // failure.
    EXPECT_TRUE(std::is_trivially_destructible<Option<NoDestructor>>::value);
    EXPECT_TRUE(std::is_trivially_destructible<Option<int>>::value);
    EXPECT_FALSE(std::is_trivially_destructible<Option<WithDestructor>>::value);
}

TEST(Option, Hash) {
    // Test it's usable in std::unordered map (compile time check)
    std::unordered_map<Option<int>, Option<int>> obj;
    // Also check the std::hash template can be instantiated by the compiler
    std::hash<Option<int>>()(None);
    std::hash<Option<int>>()(3);
}

struct WithConstMember {
    WithConstMember(int val) : x(val) {}
    const int x;
};

// Make this opaque to the optimizer by preventing inlining.
void replaceWith2(Option<WithConstMember>& o) {
    o.emplace(2);
}

TEST(Option, ConstMember) {
    // Verify that the compiler doesn't optimize out the second load of
    // o->x based on the assumption that the field is const.
    //
    // Current Option implementation doesn't defend against that
    // assumption, thus replacing an optional where the object has const
    // members is technically UB and would require wrapping each access
    // to the storage with std::launder, but this prevents useful
    // optimizations.
    //
    // Implementations of std::optional in both libstdc++ and libc++ are
    // subject to the same UB. It is then reasonable to believe that
    // major compilers don't rely on the constness assumption.
    Option<WithConstMember> o(1);
    int sum = 0;
    sum += o->x;
    replaceWith2(o);
    sum += o->x;
    EXPECT_EQ(sum, 3);
}
