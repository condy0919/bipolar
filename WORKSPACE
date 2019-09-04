workspace(name = 'bipolar')

load('@bazel_tools//tools/build_defs/repo:git.bzl', 'git_repository')
load('//bazel:dependencies.bzl', 'bipolar_dependencies')
bipolar_dependencies()

git_repository(
    name = 'com_github_nelhage_rules_boost',
    commit = '82ae1790cef07f3fd618592ad227fe2d66fe0b31',
    remote = 'https://github.com/nelhage/rules_boost',
)

load('@com_github_nelhage_rules_boost//:boost/boost.bzl', 'boost_deps')
boost_deps()
