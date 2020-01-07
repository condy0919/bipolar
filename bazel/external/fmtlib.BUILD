licenses(["notice"])

cc_library(
    name = "fmtlib",
    srcs = glob([
        "fmt/*.cc",
    ]),
    hdrs = glob([
        "include/fmt/*.h",
    ]),
    defines = ["FMT_HEADER_ONLY"],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
)
