workspace(name = "bipolar")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("//bazel:dependencies.bzl", "bipolar_dependencies")

bipolar_dependencies()

git_repository(
    name = "com_github_nelhage_rules_boost",
    commit = "dc3655d95f3d5b2e790517591a8c2672da4b6a7e",
    remote = "https://github.com/nelhage/rules_boost",
    shallow_since = "1568659902 -0700",
)

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")

boost_deps()
