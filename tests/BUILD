cc_test(
    name = 'core_test',
    srcs = [
        'option_test.cpp',
        'result_test.cpp',
        'traits_test.cpp',
    ],
    copts = ['-std=c++17', '-pipe', '-Wall', '-Wextra'],
    deps = [
        '//:bipolar',
        '@gtest//:gtest_main',
    ]
)
