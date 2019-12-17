#include "bipolar/core/logger.hpp"

#include <algorithm>
#include <string_view>
#include <vector>

#include <spdlog/sinks/stdout_sinks.h>

namespace bipolar {
Logger::Logger(const std::string& name)
    : logger_(std::make_shared<spdlog::logger>(
          name, std::make_shared<spdlog::sinks::stderr_sink_mt>())) {
    logger_->set_pattern(DEFAULT_LOG_FORMAT);
    logger_->set_level(spdlog::level::trace);

    // Ensures that ERROR/CRITICAL messages get flushed
    logger_->flush_on(spdlog::level::err);
}

Logger* Registry::try_get_logger(std::string_view s) {
    for (auto& logger : get_all_loggers()) {
        if (logger.name() == s) {
            return &logger;
        }
    }
    return nullptr;
}

spdlog::logger& Registry::get_logger(LoggerID id) {
    return *get_all_loggers()[static_cast<std::size_t>(id)].logger_;
}

void Registry::set_level(spdlog::level::level_enum level) {
    for (auto& logger : get_all_loggers()) {
        logger.logger_->set_level(level);
    }
}

void Registry::set_format(const std::string& fmt) {
    for (auto& logger : get_all_loggers()) {
        logger.logger_->set_pattern(fmt);
    }
}
} // namespace bipolar
