load('@bazel_tools//tools/build_defs/repo:git.bzl', 'git_repository',
     'new_git_repository')

def bipolar_dependencies():
    _com_github_google_googletest()
    _com_github_google_benchmark()
    _com_github_axboe_liburing()


def _com_github_google_googletest():
    git_repository(
        name = 'gtest',
        remote = 'https://github.com/google/googletest',
        tag = 'release-1.8.1',
    )

def _com_github_google_benchmark():
    git_repository(
        name = 'benchmark',
        remote = 'https://github.com/google/benchmark',
        tag = 'v1.5.0',
    )

def _com_github_axboe_liburing():
    new_git_repository(
        name = 'liburing',
        remote = 'https://github.com/axboe/liburing',
        commit = '6e9dd0c8c50b5988a0c77532c9c2bd6afd4790d2',
        build_file = '@bipolar//bazel/external:liburing.BUILD'
    )

