core
========

# [Option](option.hpp)

`Option` type represents an optional value. Either one is `Some` and contains a value, or `None`, and does not.

It has many usages:
- Initial values
- Return value for otherwise reporting simple errors, where `None` is returned on error
- Optional struct fields
- Optional function arguments

Some examples:

```cpp
Option<double> divide(int numerator, int denominator) {
    if (denominator == 0) {
        return None;
    } else {
        return Some(1.0 * numerator / denominator);
    }
}

const auto result = divide(1, 0);
assert(!result.has_value());
```

Indicates an error:

The old style: an out parameter and returns a `bool` value

```cpp
bool find(const std::vector<int>& vec, int target, std::size_t& out) {
    for (std::size_t i = 0; i < vec.size(); ++i) {
        if (vec[i] == target) {
            out = i;
            return true;
        }
    }
    return false;
}
```

With the help of `Option`: only returns `Option<T>`

```cpp
Option<std::size_t> find(const std::vector<int>& vec, int target) {
    for (std::size_t i = 0; i < vec.size(); ++i) {
        if (vec[i] == target) {
            return Some(i);
        }
    }
    return None;
}
```

# [Result](result.hpp)

`Result` is a type that represents success, failure or pending. And it can
be used for returning and propagating errors.

It's a `std::pair` like class with the variants:
- `Ok`, representing success and containing a value
- `Err`, representing error and containing an error
- `Pending`, representing pending...

Functions return `Result` whenever errors are expected and recoverable.

A simple function returning `Result` might be defined and used like so:

```cpp
Result<int, const char*> parse(const char* s) {
    if (std::strlen(s) < 3) {
        return Err<const char*>("string length is less than 3");
    }
    return Ok((s[0] - '0') * 100 + (s[1] - '0') * 10 + (s[2] - '0'));
}
```

The caller can handle the `Result` chainingly

```cpp
auto r1 = parse("123").map([](int x) { return x + 1; });
assert(r1.is_ok());
assert(r1.value() == 124);

auto r2 = parse("12").map([](int x) { return x + 1; });
assert(r2.is_error());

auto r3 = parse("12").map_err([](const char* s) {
    return s[0];
});
assert(r3.is_error());
assert(r3.error() == 's');
```

Or use `and_then` and `or_else`

```cpp
auto r1 = parse("123").and_then([](int x) -> Result<int, const char*> {
    return Ok(x + 1);
});
assert(r1.is_ok());
assert(r1.value() == 124);

auto r2 = parse("12").and_then([](int x) -> Result<int, const char*> {
    return Ok(x + 1);
});
assert(r2.is_error());

auto r3 = parse("12").or_else([](const char* s) -> Result<int, char> {
    return s[0];
});
assert(r3.is_error());
assert(r3.error() == 's');
```

Read the source code for more conbinators.

# [Void](void.hpp)

The **regular** `Void` type is like the unit type in other functional languages.

Hopes [p0146](http://wg21.link/p0146) goes well.

# [Function](function.hpp)

`Function` is a general-purpose polymorphic function wrapper.
Instances of `Function` can store, move, and invoke any callable targets.

`Function` is similar to `std::function` without `CopyConstructible` and
`CopyAssignable` requirements of callable targets. As a result, `Function`
is not copyable.

The stored callable object is called the target of `Function`.
If a `Function` contains no target, it is called empty. Invoking the target
of an empty `Function` results in `std::bad_function_call` exception being
thrown.

`std::function` does an invocable check everytime while `Function` doesn't.

It makes a trade-off between performance and cornor cases.

Calling a `Function` object constructed with `NULL` function pointer is **UB**.

```cpp
// don't please
Function<int(int)> add1((int (*)(int))nullptr);

// yes, it's true
assert(static_cast<bool>(add1));

// UB, mostly segment fault
// assert(add1(-1) == 0);
```

To get a benchmark result, run

```bash
bazel run //bipolar/core:function_benchmark -c opt
```

|     Benchmark     |     Time      |    CPU    |   Iterations   |
|-------------------|---------------|-----------|----------------|
|std::function      |    2.67 ns    |  2.67 ns  |   261619176    |
|bipolar::Function  |    1.79 ns    |  1.79 ns  |   394390281    |

# [FunctionRef](function_ref.hpp)

`FunctionRef` is an efficient, type-erasing, non-owning reference to
a callable. This is intended for use as the type of a function parameter
that is not used after the function in question returns.

This class does not own the callable, so it is not in general safe to store
a `FunctionRef`.

- To function pointer arguments, `FunctionRef` supports stateful function
- To templated arguments, `FunctionRef` is easier to write and constrained
- To `std::function`, `bipolar::Function` or other polymorphic function wrappers, `FunctionRef` has no allocations

See [p0792](http://wg21.link/p0792) for more information.


# Others

- [Overload](overload.hpp) implements [p0051](http://wg21.link/p0051)
- [byteorder utilities](byteorder.hpp) such as `htons`, `htonl`, etc...
- [Movable](movable.hpp) is similar to `boost::noncopyable` but `MoveConstructible` and `MoveAssignable`
- [ScopeGuard*](scope_guard.hpp) drop-in replacement for `boost.scope_exit`
