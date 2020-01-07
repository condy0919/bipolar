licenses(["notice"])

cc_library(
    name = "spdlog",
    hdrs = glob([
        "include/**/*.cc",
        "include/**/*.h",
    ]),
    defines = ["SPDLOG_FMT_EXTERNAL"],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
    deps = ["@fmtlib"],
)
