cc_library(
    name = "liburing",
    srcs = [
        "src/queue.c",
        "src/register.c",
        "src/setup.c",
        "src/syscall.c",
    ],
    hdrs = [
        "src/include/liburing.h",
        "src/include/liburing/barrier.h",
        "src/include/liburing/compat.h",
        "src/include/liburing/io_uring.h",
    ],
    linkstatic = True,
    strip_include_prefix = "src/include/",
    visibility = ["//visibility:public"],
)
