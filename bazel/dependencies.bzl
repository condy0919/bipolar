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
    """
    linux/io_uring.h shipped by distribution is old
    """
    new_git_repository(
        name = 'liburing',
        remote = 'https://github.com/axboe/liburing',
        commit = '556960942eaa69fd53544932f00db3fa9f196e00',
        build_file = '@bipolar//bazel/external:liburing.BUILD'
    )

