"""
Stole from `abseil-cpp` project
"""

load(
    '//bipolar:copts/copts.bzl',
    'BIPOLAR_GCC_EXCEPTIONS_FLAGS',
    'BIPOLAR_GCC_FLAGS',
    'BIPOLAR_GCC_TEST_FLAGS',
    'BIPOLAR_LLVM_EXCEPTIONS_FLAGS',
    'BIPOLAR_LLVM_FLAGS',
    'BIPOLAR_LLVM_TEST_FLAGS',
)

BIPOLAR_DEFAULT_COPTS = select({
    '//bipolar:llvm_compiler': BIPOLAR_LLVM_FLAGS,
    '//conditions:default': BIPOLAR_GCC_FLAGS,
})

BIPOLAR_TEST_COPTS = select({
    '//bipolar:llvm_compiler': BIPOLAR_LLVM_TEST_FLAGS,
    '//conditions:default': BIPOLAR_GCC_TEST_FLAGS,
})

BIPOLAR_EXCEPTION_FLAG = select({
    '//bipolar:llvm_compiler': BIPOLAR_LLVM_EXCEPTIONS_FLAGS,
    '//conditions:default': BIPOLAR_GCC_EXCEPTIONS_FLAGS,
})

BIPOLAR_DEFAULT_LINKOPTS = select({
    '//conditions:default': [],
})
