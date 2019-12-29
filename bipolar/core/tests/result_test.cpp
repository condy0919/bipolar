#include "bipolar/core/result.hpp"
#include "bipolar/core/void.hpp"
#include <string>
#include <ostream>
#include <memory>
#include <gtest/gtest.h>

using namespace bipolar;
using namespace std::literals;

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
    EXPECT_FALSE(x.is_ok());
    EXPECT_TRUE(bool(x));
    EXPECT_EQ(42, x.error());
}

TEST(Result, String) {
    Result<std::string, int> x = Ok("hello"s);
    EXPECT_TRUE(bool(x));
    EXPECT_EQ(*x, "hello");
}

TEST(Result, Const) {
    // default construct
    Result<const int, int> ex = Err(0);
    EXPECT_FALSE(ex.is_ok());
    EXPECT_EQ(0, ex.error());
    ex.emplace(4);
    EXPECT_EQ(4, *ex);
    ex.emplace(5);
    EXPECT_EQ(5, *ex);
    ex = Err(42);
    EXPECT_FALSE(ex.is_ok());
    EXPECT_EQ(42, ex.error());

    // no assignment allowed
    EXPECT_FALSE((std::is_copy_assignable<Result<const int, int>>::value));
}

TEST(Result, Simple) {
    Result<int, int> ex = Err(0);
    EXPECT_FALSE(ex.is_ok());
    EXPECT_EQ(42, ex.value_or(42));
    ex.emplace(4);
    EXPECT_TRUE(bool(ex));
    EXPECT_EQ(4, *ex);
    EXPECT_EQ(4, ex.value_or(42));
    EXPECT_EQ(4, ex.value_or_else([](int err) { return err + 1; }));
    ex = Err(-1);
    EXPECT_FALSE(ex.is_ok());
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
    auto ptr = Result<std::unique_ptr<int, ExpectingDeleter>, int>(
                   Ok(std::unique_ptr<int, ExpectingDeleter>(
                       new int(42), ExpectingDeleter{1337})))
                   .value();
    *ptr = 1337;
}

TEST(Result, dereference_move) {
    auto ptr = *Result<std::unique_ptr<int, ExpectingDeleter>, int>(
        Ok(std::unique_ptr<int, ExpectingDeleter>(new int(42),
                                                  ExpectingDeleter{1337})));
    *ptr = 1337;
}

TEST(Result, EmptyConstruct) {
    Result<int, int> ex = Err(42);
    EXPECT_FALSE(ex.is_ok());
    Result<int, int> test1(ex);
    EXPECT_FALSE(test1.is_ok());
    Result<int, int> test2(std::move(ex));
    EXPECT_FALSE(test2.is_ok());
    EXPECT_EQ(42, test2.error());
}

TEST(Result, Unique) {
    Result<std::unique_ptr<int>, int> ex = Err(-1);
    EXPECT_FALSE(ex.is_ok());
    // empty->emplaced
    ex.emplace(new int(5));
    EXPECT_TRUE(bool(ex));
    EXPECT_EQ(5, **ex);

    ex = Err(-1);
    // empty->moved
    ex = Ok(std::make_unique<int>(6));
    EXPECT_EQ(6, **ex);
    // full->moved
    ex = Ok(std::make_unique<int>(7));
    EXPECT_EQ(7, **ex);

    // move it out by move construct
    Result<std::unique_ptr<int>, int> moved(std::move(ex));
    EXPECT_TRUE(bool(moved));
    EXPECT_TRUE(ex.is_pending());
    EXPECT_EQ(7, **moved);

    EXPECT_TRUE(bool(moved));
    ex = std::move(moved); // move it back by move assign
    EXPECT_TRUE(moved.is_pending());
    EXPECT_TRUE(ex.is_ok());
    EXPECT_EQ(7, **ex);
}

TEST(Result, Shared) {
    std::shared_ptr<int> ptr;
    Result<std::shared_ptr<int>, int> ex = Err(-1);
    EXPECT_FALSE(ex.is_ok());
    // empty->emplaced
    ex.emplace(new int(5));
    EXPECT_TRUE(bool(ex));
    ptr = ex.value();
    EXPECT_EQ(ptr.get(), ex->get());
    EXPECT_EQ(2, ptr.use_count());
    ex = Err(-1);
    EXPECT_EQ(1, ptr.use_count());
    // full->copied
    ex = Ok(ptr);
    EXPECT_EQ(2, ptr.use_count());
    EXPECT_EQ(ptr.get(), ex->get());
    ex = Err(-1);
    EXPECT_EQ(1, ptr.use_count());
    // full->moved
    ex = Ok(std::move(ptr));
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

TEST(Result, SwapMethod) {
    Result<std::string, int> a = Err(0);
    Result<std::string, int> b = Err(0);

    a.swap(b);
    EXPECT_FALSE(a.is_ok());
    EXPECT_FALSE(b.is_ok());

    a = Ok("hello"s);
    EXPECT_TRUE(a.is_ok());
    EXPECT_FALSE(b.is_ok());
    EXPECT_EQ("hello", a.value());

    b.swap(a);
    EXPECT_FALSE(a.is_ok());
    EXPECT_TRUE(b.is_ok());
    EXPECT_EQ("hello", b.value());

    a = Ok("bye"s);
    EXPECT_TRUE(a.is_ok());
    EXPECT_EQ("bye", a.value());

    a.swap(b);
    EXPECT_EQ("hello", a.value());
    EXPECT_EQ("bye", b.value());
}

TEST(Result, StdSwapFunction) {
    Result<std::string, int> a = Err(0);
    Result<std::string, int> b = Err(1);

    std::swap(a, b);
    EXPECT_FALSE(a.is_ok());
    EXPECT_FALSE(b.is_ok());

    a = Ok("greeting"s);
    EXPECT_TRUE(a.is_ok());
    EXPECT_FALSE(b.is_ok());
    EXPECT_EQ("greeting", a.value());

    std::swap(a, b);
    EXPECT_FALSE(a.is_ok());
    EXPECT_TRUE(b.is_ok());
    EXPECT_EQ("greeting", b.value());

    a = Ok("goodbye"s);
    EXPECT_TRUE(a.is_ok());
    EXPECT_EQ("goodbye", a.value());

    std::swap(a, b);
    EXPECT_EQ("greeting", a.value());
    EXPECT_EQ("goodbye", b.value());
}

TEST(Result, Comparisons) {
    Result<int, int> o_ = Err(0);
    Result<int, int> o1 = Ok(1);
    Result<int, int> o2 = Ok(2);

    EXPECT_TRUE(o_ == (o_));

    EXPECT_TRUE(o1 == (o1));
    EXPECT_TRUE(o1 != o2);

    EXPECT_FALSE(o2 == o1);
    EXPECT_FALSE(o1 != (o1));
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
    EXPECT_TRUE(b);

    // Truthy tests work and are not ambiguous
    if (mbool && mshort && mstr && mint) {         // only checks not-empty
    }

    mbool = Ok(false);
    EXPECT_TRUE(bool(mbool));
    EXPECT_FALSE(*mbool);

    mbool = Ok(true);
    EXPECT_TRUE(bool(mbool));
    EXPECT_TRUE(*mbool);
}

TEST(Result, MakeOptional) {
    // const L-value version
    const std::string s("abc");
    Result<std::string, int> exStr = Ok(s);
    ASSERT_TRUE(exStr.is_ok());
    EXPECT_EQ(*exStr, "abc");
    *exStr = "cde";
    EXPECT_EQ(s, "abc");
    EXPECT_EQ(*exStr, "cde");

    // L-value version
    std::string s2("abc");
    Result<std::string, int> exStr2 = Ok(s2);
    ASSERT_TRUE(exStr2.is_ok());
    EXPECT_EQ(*exStr2, "abc");
    *exStr2 = "cde";
    // it's vital to check that s2 wasn't clobbered
    EXPECT_EQ(s2, "abc");

    // L-value reference version
    std::string& s3(s2);
    Result<std::string, int> exStr3 = Ok(s3);
    ASSERT_TRUE(exStr3.is_ok());
    EXPECT_EQ(exStr3.value(), "abc");
    *exStr3 = "cde";
    EXPECT_EQ(s3, "abc");

    // R-value ref version
    std::unique_ptr<int> pInt(new int(3));
    Result<std::unique_ptr<int>, int> exIntPtr = Ok(std::move(pInt));
    EXPECT_TRUE(pInt.get() == nullptr);
    ASSERT_TRUE(exIntPtr.is_ok());
    EXPECT_EQ(*exIntPtr.value(), 3);
}

class ContainsResult {
public:
   ContainsResult() : ex_(Err(0)) {}
   explicit ContainsResult(int x) : ex_(Ok(x)) {}

   bool is_ok() const {
       return ex_.is_ok();
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
        EXPECT_TRUE(target.is_ok());
        EXPECT_EQ(5, target.value());
    }

    {
        ContainsResult source(5), target;
        target = std::move(source);
        EXPECT_TRUE(target.is_ok());
        EXPECT_EQ(5, target.value());
        EXPECT_FALSE(source.is_ok());
    }

    {
        ContainsResult ex_uninit, target(10);
        target = ex_uninit;
        EXPECT_FALSE(target.is_ok());
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
        EXPECT_FALSE(ex.is_ok());
        EXPECT_FALSE(ex.is_pending());
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
        auto ex =
            Result<std::string, int>(Ok("233"s)).map([](std::string s) -> int {
                return std::stoi(s);
            });
        EXPECT_TRUE(bool(ex));
        EXPECT_EQ(233, *ex);

        auto ex2 = Result<int, std::string>(Err("233"s)).map([](int x) -> int {
            return x + 1;
        });
        EXPECT_FALSE(ex2.is_ok());
        EXPECT_FALSE(ex2.is_pending());
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
        auto ex =
            Result<int, std::string>(Err("233"s))
                .map_err([](std::string s) -> int { return std::stoi(s); });
        EXPECT_FALSE(ex.is_ok());
        EXPECT_FALSE(ex.is_pending());
        EXPECT_EQ(233, ex.error());

        auto ex2 =
            Result<std::string, int>(Ok("233"s)).map_err([](int x) -> int {
                return x + 1;
            });
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
