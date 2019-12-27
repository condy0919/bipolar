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

the old style: an out parameter and returns a `bool` value
with `Option`: only returns `Option<T>`

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
