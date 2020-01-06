//!
//!
//!

#ifndef BIPOLAR_CORE_ASSERT_HPP_
#define BIPOLAR_CORE_ASSERT_HPP_

#include "bipolar/core/logger.hpp"

#ifndef NDEBUG
#define BIPOLAR_ASSERT(expr, ...)                                              \
    do {                                                                       \
        if (!(expr)) {                                                         \
            auto& logger =                                                     \
                ::bipolar::Registry::get_logger(::bipolar::LoggerID::assert);  \
            BIPOLAR_LOG_WARN(logger, __VA_ARGS__);                             \
        }                                                                      \
    } while (false)
#else
#define BIPOLAR_ASSERT(expr, ...) ((void)(expr))
#endif

#endif
