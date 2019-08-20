#include "bipolar/result.hpp"
#include <string>
#include <ostream>
#include <memory>
#include <gtest/gtest.h>

using namespace bipolar;
using namespace std::literals;

template <typename T, typename E>
std::ostream& operator<<(std::ostream& os, const Result<T, E>& res) {
    if (res.has_value()) {
        return os << "Ok(" << res.value() << ")";
    } else {
        return os << "Err(" << res.error() << ")";
    }
}

struct NoDefault {
    NoDefault(int, int) {}
    char a, b, c;
};

TEST(Result, NoDefault) {
    Result<NoDefault, int> x = Ok(NoDefault{42, 42});
    EXPECT_TRUE(bool(x));

    x.emplace(4, 5);
    EXPECT_TRUE(bool(x));

    x = Err(42);
    EXPECT_FALSE(bool(x));
    EXPECT_EQ(42, x.error());
}

TEST(Result, String) {
    Result<std::string, int> x = Ok("hello"s);
    EXPECT_TRUE(bool(x));
    EXPECT_EQ(*x, "hello");
}

TEST(Result, Ambiguous) {
    // Potentially ambiguous and confusing construction and assignment disallowed:
    EXPECT_FALSE((std::is_constructible<Result<int, int>, int>::value));
    EXPECT_FALSE((std::is_assignable<Result<int, int>&, int>::value));
}

TEST(Result, Const) {
    // default construct
    Result<const int, int> ex = Err(0);
    EXPECT_FALSE(bool(ex));
    EXPECT_EQ(0, ex.error());
    ex.emplace(4);
    EXPECT_EQ(4, *ex);
    ex.emplace(5);
    EXPECT_EQ(5, *ex);
    ex = Err(42);
    EXPECT_FALSE(bool(ex));
    EXPECT_EQ(42, ex.error());

    // no assignment allowed
    EXPECT_FALSE((std::is_copy_assignable<Result<const int, int>>::value));
}

TEST(Result, Simple) {
    Result<int, int> ex = Err(0);
    EXPECT_FALSE(bool(ex));
    EXPECT_EQ(42, ex.value_or(42));
    ex.emplace(4);
    EXPECT_TRUE(bool(ex));
    EXPECT_EQ(4, *ex);
    EXPECT_EQ(4, ex.value_or(42));
    EXPECT_EQ(4, ex.value_or_else([](int err) { return err + 1; }));
    ex = Err(-1);
    EXPECT_FALSE(bool(ex));
    EXPECT_EQ(-1, ex.error());
    EXPECT_EQ(42, ex.value_or(42));
    EXPECT_EQ(0, ex.value_or_else([](int err) { return err + 1; }));
}

class MoveTester {
public:
    /* implicit */ MoveTester(const char* s) : s_(s) {}
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

private:
    friend bool operator==(const MoveTester& o1, const MoveTester& o2);
    std::string s_;
};

bool operator==(const MoveTester& o1, const MoveTester& o2) {
    return o1.s_ == o2.s_;
}

TEST(Result, value_or_rvalue_arg) {
    Result<MoveTester, int> ex = Err(-1);
    MoveTester dflt = "hello";
    EXPECT_EQ("hello", ex.value_or(dflt));
    EXPECT_EQ("hello", dflt);
    EXPECT_EQ("hello", ex.value_or(std::move(dflt)));
    EXPECT_EQ("", dflt);
    EXPECT_EQ("world", ex.value_or("world"));

    dflt = "hello";
    // Make sure that the const overload works on const objects
    const auto& exc = ex;
    EXPECT_EQ("hello", exc.value_or(dflt));
    EXPECT_EQ("hello", dflt);
    EXPECT_EQ("hello", exc.value_or(std::move(dflt)));
    EXPECT_EQ("", dflt);
    EXPECT_EQ("world", exc.value_or("world"));

    dflt = "hello";
    ex.emplace("meow");
    EXPECT_EQ("meow", ex.value_or(dflt));
    EXPECT_EQ("hello", dflt);
    EXPECT_EQ("meow", ex.value_or(std::move(dflt)));
    EXPECT_EQ("hello", dflt); // only moved if used
}

TEST(Result, value_or_noncopyable) {
    Result<std::unique_ptr<int>, int> ex = Err(42);
    auto dflt = std::make_unique<int>(42);
    EXPECT_EQ(42, *std::move(ex).value_or(std::move(dflt)));
}

struct ExpectingDeleter {
   explicit ExpectingDeleter(int expected_) : expected(expected_) {}
   int expected;
   void operator()(const int* ptr) {
       EXPECT_EQ(*ptr, expected);
       delete ptr;
   }
};

TEST(Result, value_move) {
    auto ptr = Ok(std::unique_ptr<int, ExpectingDeleter>(
                      new int(42), ExpectingDeleter{1337}))
                   .into<int>()
                   .value();
    *ptr = 1337;
}

TEST(Result, dereference_move) {
    auto ptr = *Ok(std::unique_ptr<int, ExpectingDeleter>(
                       new int(42), ExpectingDeleter{1337}))
                    .into<int>();
    *ptr = 1337;
}

TEST(Result, EmptyConstruct) {
    Result<int, int> ex = Err(42);
    EXPECT_FALSE(bool(ex));
    Result<int, int> test1(ex);
    EXPECT_FALSE(bool(test1));
    Result<int, int> test2(std::move(ex));
    EXPECT_FALSE(bool(test2));
    EXPECT_EQ(42, test2.error());
}

TEST(Result, Unique) {
    Result<std::unique_ptr<int>, int> ex = Err(-1);
    EXPECT_FALSE(bool(ex));
    // empty->emplaced
    ex.emplace(new int(5));
    EXPECT_TRUE(bool(ex));
    EXPECT_EQ(5, **ex);

    ex = Err(-1);
    // empty->moved
    ex = std::make_unique<int>(6);
    EXPECT_EQ(6, **ex);
    // full->moved
    ex = std::make_unique<int>(7);
    EXPECT_EQ(7, **ex);

    // move it out by move construct
    Result<std::unique_ptr<int>, int> moved(std::move(ex));
    EXPECT_TRUE(bool(moved));
    EXPECT_TRUE(bool(ex));
    EXPECT_EQ(nullptr, ex->get());
    EXPECT_EQ(7, **moved);

    EXPECT_TRUE(bool(moved));
    ex = std::move(moved); // move it back by move assign
    EXPECT_TRUE(bool(moved));
    EXPECT_EQ(nullptr, moved->get());
    EXPECT_TRUE(bool(ex));
    EXPECT_EQ(7, **ex);
}

TEST(Result, Shared) {
    std::shared_ptr<int> ptr;
    Result<std::shared_ptr<int>, int> ex = Err(-1);
    EXPECT_FALSE(bool(ex));
    // empty->emplaced
    ex.emplace(new int(5));
    EXPECT_TRUE(bool(ex));
    ptr = ex.value();
    EXPECT_EQ(ptr.get(), ex->get());
    EXPECT_EQ(2, ptr.use_count());
    ex = Err(-1);
    EXPECT_EQ(1, ptr.use_count());
    // full->copied
    ex = ptr;
    EXPECT_EQ(2, ptr.use_count());
    EXPECT_EQ(ptr.get(), ex->get());
    ex = Err(-1);
    EXPECT_EQ(1, ptr.use_count());
    // full->moved
    ex = std::move(ptr);
    EXPECT_EQ(1, ex->use_count());
    EXPECT_EQ(nullptr, ptr.get());
    {
        EXPECT_EQ(1, ex->use_count());
        Result<std::shared_ptr<int>, int> copied(ex);
        EXPECT_EQ(2, ex->use_count());
        Result<std::shared_ptr<int>, int> moved(std::move(ex));
        EXPECT_EQ(2, moved->use_count());
        moved.emplace(new int(6));
        EXPECT_EQ(1, moved->use_count());
        copied = moved;
        EXPECT_EQ(2, moved->use_count());
    }
}

TEST(Result, Order) {
    std::vector<Result<int, int>> vect{
        {Err(1)},
        {Ok(3)},
        {Ok(1)},
        {Err(2)},
        {Ok(2)},
    };
    std::vector<Result<int, int>> expected{
        {Err(1)},
        {Err(2)},
        {Ok(1)},
        {Ok(2)},
        {Ok(3)},
    };
    std::sort(vect.begin(), vect.end());
    EXPECT_EQ(vect, expected);
}

TEST(Result, SwapMethod) {
    Result<std::string, int> a = Err(0);
    Result<std::string, int> b = Err(0);

    a.swap(b);
    EXPECT_FALSE(a.has_value());
    EXPECT_FALSE(b.has_value());

    a = Ok("hello"s);
    EXPECT_TRUE(a.has_value());
    EXPECT_FALSE(b.has_value());
    EXPECT_EQ("hello", a.value());

    b.swap(a);
    EXPECT_FALSE(a.has_value());
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ("hello", b.value());

    a = Ok("bye"s);
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ("bye", a.value());

    a.swap(b);
    EXPECT_EQ("hello", a.value());
    EXPECT_EQ("bye", b.value());
}

TEST(Result, StdSwapFunction) {
    Result<std::string, int> a = Err(0);
    Result<std::string, int> b = Err(1);

    std::swap(a, b);
    EXPECT_FALSE(a.has_value());
    EXPECT_FALSE(b.has_value());

    a = Ok("greeting"s);
    EXPECT_TRUE(a.has_value());
    EXPECT_FALSE(b.has_value());
    EXPECT_EQ("greeting", a.value());

    std::swap(a, b);
    EXPECT_FALSE(a.has_value());
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ("greeting", b.value());

    a = Ok("goodbye"s);
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ("goodbye", a.value());

    std::swap(a, b);
    EXPECT_EQ("greeting", a.value());
    EXPECT_EQ("goodbye", b.value());
}

TEST(Result, Comparisons) {
    Result<int, int> o_ = Err(0);
    Result<int, int> o1 = Ok(1);
    Result<int, int> o2 = Ok(2);

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

    /* folly::Result explicitly doesn't support comparisons with contained value
    EXPECT_TRUE(1 < o2);
    EXPECT_TRUE(1 <= o2);
    EXPECT_TRUE(1 <= o1);
    EXPECT_TRUE(1 == o1);
    EXPECT_TRUE(2 != o1);
    EXPECT_TRUE(1 >= o1);
    EXPECT_TRUE(2 >= o1);
    EXPECT_TRUE(2 > o1);

    EXPECT_FALSE(o2 < 1);
    EXPECT_FALSE(o2 <= 1);
    EXPECT_FALSE(o2 <= 1);
    EXPECT_FALSE(o2 == 1);
    EXPECT_FALSE(o2 != 2);
    EXPECT_FALSE(o1 >= 2);
    EXPECT_FALSE(o1 >= 2);
    EXPECT_FALSE(o1 > 2);
    */
}

TEST(Result, Conversions) {
    Result<bool, int> mbool = Err(0);
    Result<short, int> mshort = Err(0);
    Result<char*, int> mstr = Err(0);
    Result<int, int> mint = Err(0);

    EXPECT_FALSE((std::is_convertible<Result<bool, int>&, bool>::value));
    EXPECT_FALSE((std::is_convertible<Result<short, int>&, short>::value));
    EXPECT_FALSE((std::is_convertible<Result<char*, int>&, char*>::value));
    EXPECT_FALSE((std::is_convertible<Result<int, int>&, int>::value));

    // intended explicit operator bool, for if (ex).
    bool b(mbool);
    EXPECT_FALSE(b);

    // Truthy tests work and are not ambiguous
    if (mbool && mshort && mstr && mint) {         // only checks not-empty
        if (*mbool && *mshort && *mstr && *mint) { // only checks value
            ;
        }
    }

    mbool = Ok(false);
    EXPECT_TRUE(bool(mbool));
    EXPECT_FALSE(*mbool);

    mbool = Ok(true);
    EXPECT_TRUE(bool(mbool));
    EXPECT_TRUE(*mbool);

    // No conversion allowed; does not compile
    // mbool == false;
}

TEST(Result, MakeOptional) {
    // const L-value version
    const std::string s("abc");
    Result<std::string, int> exStr = Ok(s);
    ASSERT_TRUE(exStr.has_value());
    EXPECT_EQ(*exStr, "abc");
    *exStr = "cde";
    EXPECT_EQ(s, "abc");
    EXPECT_EQ(*exStr, "cde");

    // L-value version
    std::string s2("abc");
    Result<std::string, int> exStr2 = Ok(s2);
    ASSERT_TRUE(exStr2.has_value());
    EXPECT_EQ(*exStr2, "abc");
    *exStr2 = "cde";
    // it's vital to check that s2 wasn't clobbered
    EXPECT_EQ(s2, "abc");

    // L-value reference version
    std::string& s3(s2);
    Result<std::string, int> exStr3 = Ok(s3);
    ASSERT_TRUE(exStr3.has_value());
    EXPECT_EQ(*exStr3, "abc");
    *exStr3 = "cde";
    EXPECT_EQ(s3, "abc");

    // R-value ref version
    std::unique_ptr<int> pInt(new int(3));
    Result<std::unique_ptr<int>, int> exIntPtr = Ok(std::move(pInt));
    EXPECT_TRUE(pInt.get() == nullptr);
    ASSERT_TRUE(exIntPtr.has_value());
    EXPECT_EQ(**exIntPtr, 3);
}

TEST(Result, SelfAssignment) {
    Result<std::string, int> a = Ok("42"s);
    a = static_cast<decltype(a)&>(a); // suppress self-assign warning
    ASSERT_TRUE(a.has_value() && a.value() == "42");

    Result<std::string, int> b = Ok("23333333"s);
    b = static_cast<decltype(b)&&>(b); // suppress self-move warning
    ASSERT_TRUE(b.has_value() && b.value() == "23333333");
}

class ContainsResult {
public:
   ContainsResult() : ex_(Err(0)) {}
   explicit ContainsResult(int x) : ex_(Ok(x)) {}

   bool has_value() const {
       return ex_.has_value();
   }
   int value() const {
       return ex_.value();
   }

   ContainsResult(const ContainsResult& other) = default;
   ContainsResult& operator=(const ContainsResult& other) = default;
   ContainsResult(ContainsResult&& other) = default;
   ContainsResult& operator=(ContainsResult&& other) = default;

private:
   Result<int, int> ex_;
};

/**
 * Test that a class containing an Result can be copy and move assigned.
 * This was broken under gcc 4.7 until assignment operators were explicitly
 * defined.
 */
TEST(Result, AssignmentContained) {
    {
        ContainsResult source(5), target;
        target = source;
        EXPECT_TRUE(target.has_value());
        EXPECT_EQ(5, target.value());
    }

    {
        ContainsResult source(5), target;
        target = std::move(source);
        EXPECT_TRUE(target.has_value());
        EXPECT_EQ(5, target.value());
        EXPECT_TRUE(source.has_value());
    }

    {
        ContainsResult ex_uninit, target(10);
        target = ex_uninit;
        EXPECT_FALSE(target.has_value());
    }
}

TEST(Result, Exceptions) {
    Result<int, int> bad(Err(0));
    EXPECT_THROW(bad.value(), BadResultAccess);
}

struct ThrowingBadness {
    ThrowingBadness() noexcept(false);
    ThrowingBadness(const ThrowingBadness&) noexcept(false);
    ThrowingBadness(ThrowingBadness&&) noexcept(false);
    ThrowingBadness& operator=(const ThrowingBadness&) noexcept(false);
    ThrowingBadness& operator=(ThrowingBadness&&) noexcept(false);
};

TEST(Result, NoThrowMoveConstructible) {
    EXPECT_TRUE((std::is_nothrow_move_constructible<Result<bool, int>>::value));
    EXPECT_TRUE((std::is_nothrow_move_constructible<
                 Result<std::unique_ptr<int>, int>>::value));
    EXPECT_FALSE((std::is_nothrow_move_constructible<
                  Result<ThrowingBadness, int>>::value));
}

TEST(Result, NoThrowMoveAssignable) {
    EXPECT_TRUE((std::is_nothrow_move_assignable<Result<bool, int>>::value));
    EXPECT_TRUE((std::is_nothrow_move_assignable<
                 Result<std::unique_ptr<int>, int>>::value));
    EXPECT_FALSE(
        (std::is_nothrow_move_assignable<Result<ThrowingBadness, int>>::value));
}

struct NoSelfAssign {
    NoSelfAssign() = default;
    NoSelfAssign(NoSelfAssign&&) = default;
    NoSelfAssign(const NoSelfAssign&) = default;
    NoSelfAssign& operator=(NoSelfAssign&& that) {
        EXPECT_NE(this, &that);
        return *this;
    }
    NoSelfAssign& operator=(const NoSelfAssign& that) {
        EXPECT_NE(this, &that);
        return *this;
    }
};

TEST(Result, NoSelfAssign) {
    Result<NoSelfAssign, int> e{Ok(NoSelfAssign{})};
    e = static_cast<decltype(e)&>(e);  // suppress self-assign warning
    e = static_cast<decltype(e)&&>(e); // @nolint suppress self-move warning
}

struct NoDestructor {};

struct WithDestructor {
    ~WithDestructor();
};

TEST(Result, TriviallyDestructible) {
    // These could all be static_asserts but EXPECT_* give much nicer output on
    // failure.
    EXPECT_TRUE(
        (std::is_trivially_destructible<Result<NoDestructor, int>>::value));
    EXPECT_TRUE((std::is_trivially_destructible<Result<int, int>>::value));
    EXPECT_FALSE(
        (std::is_trivially_destructible<Result<WithDestructor, int>>::value));
}

struct NoConstructor {};

struct WithConstructor {
    WithConstructor();
};

TEST(Result, AndThenOrElse) {
    // Flattening
    {
        auto ex =
            Ok(std::make_unique<int>(42))
                .into<int>()
                .and_then([](std::unique_ptr<int> p) -> Result<int, int> {
                    return Ok(*p);
                });
        EXPECT_TRUE(bool(ex));
        EXPECT_EQ(42, *ex);
    }

    // Void
    {
        auto ex =
            Ok(std::make_unique<int>(42))
                .into<int>()
                .and_then([](std::unique_ptr<int>) -> Result<Void, int> {
                    return Ok(Void());
                });
        EXPECT_TRUE(bool(ex));
    }

    // Chaining
    {
        auto ex = Ok(std::make_unique<int>(42))
                      .into<int>()
                      .and_then([](std::unique_ptr<int> p) -> Result<int, int> {
                          return Ok(*p);
                      })
                      .and_then([](int i) -> Result<std::string, int> {
                          return i == 42 ? Ok("yes"s) : Ok("no"s);
                      });
        EXPECT_TRUE(bool(ex));
        EXPECT_EQ("yes", *ex);
    }

    // Chaining with errors
    {
        auto ex = Ok(std::make_unique<int>(42))
                      .into<int>()
                      .and_then([](std::unique_ptr<int>) -> Result<int, int> {
                          return Err(-1);
                      })
                      .and_then([](int i) -> Result<std::string, int> {
                          return i == 42 ? Ok("yes"s) : Ok("no"s);
                      });
        EXPECT_FALSE(bool(ex));
        EXPECT_EQ(-1, ex.error());
    }

    {
        auto ex = Ok(std::make_unique<int>(42))
                      .into<int>()
                      .and_then([](std::unique_ptr<int> p) -> Result<int, int> {
                          return Ok(*p);
                      })
                      .or_else([](int) -> Result<int, int> { throw "123"; });
        EXPECT_TRUE(bool(ex));
        EXPECT_EQ(42, *ex);
    }

    {
        EXPECT_THROW(Err("123"s).into<int>().or_else(
                         [](std::string s) -> Result<int, int> {
                             throw std::runtime_error(std::move(s));
                         }),
                     std::runtime_error);
    }
}

TEST(Result, Map) {
    {
        auto ex = Ok("233"s).into<int>().map(
            [](std::string s) -> int { return std::stoi(s); });
        EXPECT_TRUE(bool(ex));
        EXPECT_EQ(233, *ex);

        auto ex2 =
            Err("233"s).into<int>().map([](int x) -> int { return x + 1; });
        EXPECT_FALSE(bool(ex2));
        EXPECT_EQ(ex2.error(), "233");
    }

    {
        int m = Err("233"s).into<int>().map_or_else(
            [](std::string e) -> int { return std::stoi(e); },
            [](int x) -> int { return x + 1; });
        EXPECT_EQ(m, 233);

        int m2 = Ok("233"s).into<int>().map_or_else(
            [](int x) -> int { return x + 1; },
            [](std::string e) -> int { return std::stoi(e); });
        EXPECT_EQ(m2, 233);
    }

    {
        auto ex = Err("233"s).into<int>().map_err(
            [](std::string s) -> int { return std::stoi(s); });
        EXPECT_FALSE(bool(ex));
        EXPECT_EQ(233, ex.error());

        auto ex2 =
            Ok("233"s).into<int>().map_err([](int x) -> int { return x + 1; });
        EXPECT_TRUE(bool(ex2));
        EXPECT_EQ("233", *ex2);
    }
}

TEST(Result, Contains) {
    const Result<std::string, int> x = Ok("233"s);
    EXPECT_TRUE(x.contains("233"));
    EXPECT_FALSE(x.contains_err(233));


    const Result<std::string, int> y = Err(-1);
    EXPECT_TRUE(y.contains_err(-1));
    EXPECT_FALSE(y.contains(""));
}

TEST(Result, OkErr) {
    const Result<std::string, int> x = Ok("233"s), y = Err(-1);
    EXPECT_EQ(x.ok(), Some("233"s));
    EXPECT_EQ(y.ok(), None);
    EXPECT_EQ(x.err(), None);
    EXPECT_EQ(y.err(), Some(-1));
}

TEST(Result, Expect) {
    const Result<std::string, int> x = Ok("233"s), y = Err(-1);

    EXPECT_EQ(x.expect("it should be 233"), "233");
    EXPECT_THROW(
        try {
            y.expect("it should be 233");
        } catch (const BadResultAccess& ex) {
            EXPECT_EQ("it should be 233"s, ex.what());
            throw;
        },
        BadResultAccess);

    EXPECT_THROW(
        try {
            x.expect_err("it should be -1");
        } catch (const BadResultAccess& ex) {
            EXPECT_EQ("it should be -1"s, ex.what());
            throw;
        },
        BadResultAccess);
    EXPECT_EQ(y.expect_err("it should be -1"), -1);
}

namespace {
struct Source {};

struct SmallPODConstructTo {
    SmallPODConstructTo() = default;

    explicit SmallPODConstructTo(Source) {}
};

struct LargePODConstructTo {
    explicit LargePODConstructTo(Source) {}

    int64_t array[10];
};

struct NonPODConstructTo {
    explicit NonPODConstructTo(Source) {}

    NonPODConstructTo(NonPODConstructTo const&) {}

    NonPODConstructTo& operator=(NonPODConstructTo const&) {
        return *this;
    }
};

struct ConvertTo {
    explicit ConvertTo(Source) {}

    ConvertTo& operator=(Source) {
        return *this;
    }
};

static_assert(
    std::is_same_v<detail::ResultStorage<int, SmallPODConstructTo>,
                   detail::SmallTrivialStorage<int, SmallPODConstructTo>>);

static_assert(std::is_same_v<detail::ResultStorage<int, LargePODConstructTo>,
                             detail::TrivialStorage<int, LargePODConstructTo>>);

static_assert(
    std::is_same_v<detail::ResultStorage<int, NonPODConstructTo>,
                   detail::NonTrivialStorage<int, NonPODConstructTo>>);

template <typename From, typename To>
struct IsConvertible : std::bool_constant<std::is_convertible_v<From, To> &&
                                          std::is_assignable_v<To&, From>> {};

template <typename Target>
constexpr bool constructibleNotConvertible() {
    return std::is_constructible<Target, Source>() &&
           !IsConvertible<Source, Target>();
}

static_assert(constructibleNotConvertible<SmallPODConstructTo>());
static_assert(constructibleNotConvertible<LargePODConstructTo>());
static_assert(constructibleNotConvertible<NonPODConstructTo>());

//static_assert(IsConvertible<Source, ConvertTo>(), "convertible");
} // namespace

TEST(Result, GitHubIssue1111) {
    // See https://github.com/facebook/folly/issues/1111
    Result<int, SmallPODConstructTo> a = Ok(5);
    EXPECT_EQ(a.value(), 5);
}

TEST(Result, ConstructorConstructibleNotConvertible) {
    const Result<int, Source> v = Ok(5);
    const Result<int, Source> e = Err(Source());
    // Test construction and assignment for each ResultStorage backend
    {
        Result<int, SmallPODConstructTo> cv(v);
        Result<int, SmallPODConstructTo> ce(e);
        cv = v;
        ce = e;
    }

    {
        Result<int, LargePODConstructTo> cv(v);
        Result<int, LargePODConstructTo> ce(e);
        cv = v;
        ce = e;
    }

    {
        Result<int, NonPODConstructTo> cv(v);
        Result<int, NonPODConstructTo> ce(e);
        cv = v;
        ce = e;
    }

    // Test convertible construction and assignment
    {
        Result<int, ConvertTo> cv(v);
        Result<int, ConvertTo> ce(e);
        cv = v;
        ce = e;
    }
}
