[![Build Status][github-ci-badge]][github-link]
[![Codacy Badge][codacy-badge]][codacy-link]
[![Codecov Badge][codecov-badge]][codecov-link]
[![MIT License][license-badge]](LICENSE)
[![Language][language-badge]][language-link]

# bipolar

`bipolar` is a nano library aimed at concurrency programming.

## Basic

- [core](bipolar/core/README.md)
- [io](bipolar/io/README.md)
- [futures](bipolar/futures/README.md)
- [net](bipolar/net/README.md)

## How to start

`bipolar` writes in C++17, a latest version of gcc/clang is recommended.

[bazel](https://github.com/bazelbuild/bazel/) is required to build and run tests/benchmarks/examples.

### Build

```
bazel build //...
```

### Test and benchmark

Actually tests and benchmarks are all `cc_test` except a `benchmark` tag.

All:

```
bazel test //...
```

Only tests:

```
bazel test //... --test_tag_filters=-benchmark,-example
```

Only benchmarks:

```
bazel test //... --test_tag_filters=benchmark
```

Only examples:

```
bazel test //... --test_tag_filters=example
```

### Coverage

```
bazel test //... --config=kcov
```

generates code coverage reports by [kcov][kcov-link].

Checks `bazel-kcov` directory for code coverage.

## FAQ

[github-ci-badge]: https://github.com/condy0919/bipolar/workflows/BIPOLAR%20CI/badge.svg
[github-link]: https://github.com/condy0919/bipolar
[codacy-badge]: https://api.codacy.com/project/badge/Grade/7c5e88ade2944d7ca1741d2b3e709f4f
[codacy-link]: https://www.codacy.com/manual/condy0919/bipolar?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=condy0919/bipolar&amp;utm_campaign=Badge_Grade
[codecov-badge]: https://codecov.io/gh/condy0919/bipolar/branch/master/graph/badge.svg
[codecov-link]: https://codecov.io/gh/condy0919/bipolar
[license-badge]: https://img.shields.io/badge/license-MIT-007EC7.svg
[language-badge]: https://img.shields.io/badge/Language-C%2B%2B17-blue.svg
[language-link]: https://en.cppreference.com/w/cpp/compiler_support
[kcov-link]: https://github.com/SimonKagstrom/kcov/
