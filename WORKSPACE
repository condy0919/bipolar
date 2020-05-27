workspace(name = "bipolar")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("//bazel:dependencies.bzl", "bipolar_dependencies")

bipolar_dependencies()

git_repository(
    name = "com_github_nelhage_rules_boost",
    commit = "0cc5bf5513c067917b5e083cee22a8dcdf2e0266",
    remote = "https://github.com/nelhage/rules_boost",
    shallow_since = "1575786499 -0800",
)

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")

boost_deps()
