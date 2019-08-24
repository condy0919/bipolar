cc_library(
    name = 'core',
    srcs = [
        'bipolar/core/eventloop.cpp',
    ],
    hdrs = [
        'bipolar/core/option.hpp',
        'bipolar/core/result.hpp',
        'bipolar/core/void.hpp',
        'bipolar/core/traits.hpp',
        'bipolar/core/eventloop.hpp',
    ],
    copts = ['-std=c++17', '-pipe', '-Wall', '-Wextra'],
    linkopts = ['-pthread'],
    visibility = ['//visibility:public'],
    deps = [
        '@boost//:noncopyable',
    ],
)
