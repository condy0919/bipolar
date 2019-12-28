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

# [Void](void.hpp)

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
assert(add1(-1) == 0);
```

To get a benchmark result, run

```bash
bazel run //bipolar/core:function_benchmark -c opt
```

|     Benchmark     |     Time      |    CPU    |   Iterations   |
|-------------------|---------------|-----------|----------------|
|std::function      |    2.67 ns    |  2.67 ns  |   261619176    |
|bipolar::function  |    1.79 ns    |  1.79 ns  |   394390281    |

# [FunctionRef](function_ref.hpp)

# [ScopeGuard*](scope_guard.hpp)

# [thread safety attributes](thread_safety.hpp)

# Others

- [Overload](overload.hpp) implements [p0051](http://wg21.link/p0051)
- [byteorder utilities](byteorder.hpp) such as `htons`, `htonl`, etc...
- [Movable](movable.hpp) is similar to `boost::noncopyable` but `MoveConstructible` and `MoveAssignable`
- [Logger](logger.hpp)
- [likely](likely.hpp)
- [assume](assume.hpp)
- [assert](assert.hpp)
