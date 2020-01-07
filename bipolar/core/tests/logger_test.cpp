#include "bipolar/core/logger.hpp"

#include <sstream>
#include <streambuf>

#include <gtest/gtest.h>

using namespace bipolar;

TEST(Logger, nonexistent) {
    Logger* logger = Registry::try_get_logger("a_nonexistent_logger");
    EXPECT_FALSE(logger);
}

TEST(Logger, name) {
    Logger* logger = Registry::try_get_logger("assert");
    EXPECT_EQ(logger->name(), "assert");
}

TEST(Logger, level) {
    Logger* logger = Registry::try_get_logger("assert");
    EXPECT_TRUE(logger);

    EXPECT_EQ(logger->level(), spdlog::level::trace);
    EXPECT_EQ(logger->level_string_view(), "trace");

    logger->set_level(spdlog::level::info);
    EXPECT_EQ(logger->level(), spdlog::level::info);
    EXPECT_EQ(logger->level_string_view(), "info");

    // restore
    logger->set_level(spdlog::level::trace);
}

TEST(Logger, output) {
    // The default log level: trace
    auto& logger = Registry::get_logger(LoggerID::assert);

    testing::internal::CaptureStderr();

    BIPOLAR_LOG_INFO(logger, "buzz");
    BIPOLAR_LOG_FLUSH(logger);

    const std::string output = testing::internal::GetCapturedStderr();

    EXPECT_FALSE(output.empty());
    EXPECT_EQ(output.substr(output.size() - 5), "buzz\n");
}

TEST(Logger, output_suppressed) {
    // The default log level: trace
    auto& logger = Registry::get_logger(LoggerID::assert);

    logger.set_level(spdlog::level::err);

    testing::internal::CaptureStderr();

    BIPOLAR_LOG_INFO(logger, "buzz");
    BIPOLAR_LOG_FLUSH(logger);

    const std::string output = testing::internal::GetCapturedStderr();

    EXPECT_TRUE(output.empty());

    // restore
    logger.set_level(spdlog::level::trace);
}

TEST(Registry, set_level) {
    Registry::set_level(spdlog::level::info);

    Logger* logger = Registry::try_get_logger("assert");
    EXPECT_TRUE(logger);
    EXPECT_EQ(logger->level(), spdlog::level::info);

    // restore
    logger->set_level(spdlog::level::trace);
}

// It must be the last one
// We can't restore the format
TEST(Registry, set_format) {
    Registry::set_format("%%");

    auto& logger = Registry::get_logger(LoggerID::assert);

    testing::internal::CaptureStderr();

    BIPOLAR_LOG_INFO(logger, "buzz");
    BIPOLAR_LOG_FLUSH(logger);

    const std::string output = testing::internal::GetCapturedStderr();

    EXPECT_TRUE(!output.empty());
    EXPECT_EQ(output, "%\n");

    // restore
    logger.set_level(spdlog::level::trace);
}
