//! Logger
//!
//! An wrapper for spdlog
//!
//! See also `Logger` and `Registry`
//! See https://github.com/gabime/spdlog/wiki for more information.

#ifndef BIPOLAR_CORE_LOGGER_HPP_
#define BIPOLAR_CORE_LOGGER_HPP_

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include <spdlog/spdlog.h>

namespace bipolar {
namespace detail {
// Gets the basename of __FILE__
// __FILE__ will be expanded to a full path, it's annoying
constexpr const char* basename(const char* s) {
    const char* current = s;
    while (*s) {
        if (*s == '/') {
            current = s + 1;
        }
        ++s;
    }
    return current;
}
} // namespace detail

// pre-installed loggers
#define BIPOLAR_ALL_LOGGER_IDS(G)                                              \
    G(assert)                                                                  \
    G(placeholder)

#define BIPOLAR_LOGGER_GENERATOR(x) Logger(#x),
#define BIPOLAR_ENUM_GENERATOR(x) x,

// clang-format off
enum class LoggerID {
    BIPOLAR_ALL_LOGGER_IDS(BIPOLAR_ENUM_GENERATOR)
};
// clang-format on

/// Logger
///
/// An logger wrapper for spdlog logger with log **TRACE** level and will
/// print messages on **stderr**.
///
/// Buffer will get flushed on **ERROR** and **CRITICAL** log level as soon
/// as possible.
///
/// User could force logger to flush by using `BIPOLAR_LOG_FLUSH`.
///
/// # Use
///
/// `Logger` provides 6 macros:
///
/// - BIPOLAR_LOG_TRACE
/// - BIPOLAR_LOG_DEBUG
/// - BIPOLAR_LOG_INFO
/// - BIPOLAR_LOG_WARN
/// - BIPOLAR_LOG_ERROR
/// - BIPOLAR_LOG_CRITICAL
///
/// where `CRITICAL` represents the highest-priority log message and `TRACE`
/// the lowest. The log messages are filtered by configuring the log level to
/// suppress messages with a lower priority.
///
/// The first parameter is a `spdlog::logger` which can be obtained from
/// `Registry::get_logger()` and `Logger::native()`.
/// The rest are format strings using [`fmtlib`].
///
/// The log level and format can be configured through `Logger::set_level()`
/// and `Logger::set_format()`.
///
/// # Examples
///
/// ```
/// class foo {
/// public:
///     void create_table(const std::string& s) {
///         // converts to spdlog::logger
///         spdlog::logger& l = logger_.native();
///
///         // prints a traced message
///         BIPOLAR_LOG_TRACE(l, "create table {}", s);
///
///         // performs...
///         const int ret = performs(...);
///         if (ret == 0) {
///             BIPOLAR_LOG_TRACE(l, "create table {} successfully", s);
///         } else {
///             BIPOLAR_LOG_WARN(l, "create table {} failed: {}", s, ret);
///         }
///     }
///
/// private:
///     Logger logger_; // actually it cannot be constructed
/// };
/// ```
///
/// # Reference
///
/// [`fmtlib`]: https://fmt.dev/latest/index.html
///
class Logger {
    friend class Registry;

public:
    /// The default log format.
    /// [2019-12-16 23:55:59.678][1232][info][assert][a.cpp:123] text
    ///  ^                   ^    ^     ^     ^       ^          ^
    ///  datetime            ms   tid   lv    name    src_loc
    ///
    /// See also https://github.com/gabime/spdlog/wiki/3.-Custom-formatting
    static constexpr const char* DEFAULT_LOG_FORMAT =
        "[%Y-%m-%d %T.%e][%t][%l][%n][%@] %v";

    /// Returns the log level
    auto level() const {
        return logger_->level();
    }

    /// Returns the log level in `string_view` type
    auto level_string_view() const {
        return spdlog::level::level_string_views[logger_->level()];
    }

    /// Sets the minimum log level
    void set_level(spdlog::level::level_enum level) {
        logger_->set_level(level);
    }

    /// Sets the log format
    void set_format(const std::string& fmt);

    /// Returns the logger's name
    const std::string& name() const {
        return logger_->name();
    }

    /// Returns the underlying spdlog logger
    spdlog::logger& native() {
        return *logger_;
    }

private:
    explicit Logger(const std::string& name);

    std::shared_ptr<spdlog::logger> logger_;
};

/// A registry of all installed loggers
class Registry {
public:
    /// Returns the logger for a given name.
    /// `nullptr` if not found
    static Logger* try_get_logger(std::string_view s);

    /// Returns the underlying spdlog logger for a given ID
    static spdlog::logger& get_logger(LoggerID id);

    /// Sets the minimum log severity for all loggers
    static void set_level(spdlog::level::level_enum level);

    /// Sets the log format for all loggers
    static void set_format(const std::string& fmt);

    /// Returns the pre-installed loggers
    static std::vector<Logger>& get_all_loggers() {
        // clang-format off
        static std::vector<Logger> loggers = {
            BIPOLAR_ALL_LOGGER_IDS(BIPOLAR_LOGGER_GENERATOR)
        };
        // clang-format on

        return loggers;
    }
};

#undef BIPOLAR_ENUM_GENERATOR
#undef BIPOLAR_LOGGER_GENERATOR
#undef BIPOLAR_ALL_LOGGER_IDS
} // namespace bipolar

#define BIPOLAR_LOG(Logger, Level, ...)                                        \
    do {                                                                       \
        using LoggerType = std::remove_reference_t<decltype(Logger)>;          \
        static_assert(std::is_same_v<LoggerType, ::spdlog::logger>,            \
                      "Logger must be spdlog::logger type");                   \
                                                                               \
        ::spdlog::source_loc src_loc(::bipolar::detail::basename(__FILE__),    \
                                     __LINE__, __func__);                      \
        Logger.log(src_loc, ::spdlog::level::Level, __VA_ARGS__);              \
    } while (false)

#define BIPOLAR_LOG_TRACE(Logger, ...) BIPOLAR_LOG(Logger, trace, __VA_ARGS__)
#define BIPOLAR_LOG_DEBUG(Logger, ...) BIPOLAR_LOG(Logger, debug, __VA_ARGS__)
#define BIPOLAR_LOG_INFO(Logger, ...) BIPOLAR_LOG(Logger, info, __VA_ARGS__)
#define BIPOLAR_LOG_WARN(Logger, ...) BIPOLAR_LOG(Logger, warn, __VA_ARGS__)
#define BIPOLAR_LOG_ERROR(Logger, ...) BIPOLAR_LOG(Logger, err, __VA_ARGS__)
#define BIPOLAR_LOG_CRITICAL(Logger, ...)                                      \
    BIPOLAR_LOG(Logger, critical, __VA_ARGS__)

#define BIPOLAR_LOG_FLUSH(Logger)                                              \
    do {                                                                       \
        using LoggerType = std::remove_reference_t<decltype(Logger)>;          \
        static_assert(std::is_same_v<LoggerType, ::spdlog::logger>,            \
                      "Logger must be spdlog::logger type");                   \
                                                                               \
        Logger.flush();                                                        \
    } while (false)

#endif
