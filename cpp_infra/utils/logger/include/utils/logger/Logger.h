#pragma once

#include <string>
#include <spdlog/spdlog.h>

#define LOGTRACE(...) \
if (spdlog::get_level() <= SPDLOG_LEVEL_TRACE) \
    SPDLOG_TRACE(__VA_ARGS__)

#define LOGDEBUG(...) \
if (spdlog::get_level() <= SPDLOG_LEVEL_DEBUG) \
    SPDLOG_DEBUG(__VA_ARGS__)

#define LOGINFO(...) \
if (spdlog::get_level() <= SPDLOG_LEVEL_INFO) \
    SPDLOG_INFO(__VA_ARGS__)

#define LOGWARN(...) \
if (spdlog::get_level() <= SPDLOG_LEVEL_WARN) \
    SPDLOG_WARN(__VA_ARGS__)

#define LOGERROR(...) \
if (spdlog::get_level() <= SPDLOG_LEVEL_ERROR) \
    SPDLOG_ERROR(__VA_ARGS__)

#define LOGCRITICAL(...) \
if (spdlog::get_level() <= SPDLOG_LEVEL_CRITICAL) \
    SPDLOG_CRITICAL(__VA_ARGS__)

#define NVD_LOG(logLevel, logVal) \
if (spdlog::get_level() <= logLevel) \
    SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), (logLevel), (logVal))

/*
 * This macro shall be used for logging only before logger got initialized
 */
#define NVD_PRE_LOG(logLevel, logVal) \
    Log::addLogEntryToBuffer((logLevel), (logVal))

namespace nvd {

class Log
{
public:
    static void addLogEntryToBuffer(spdlog::level::level_enum logLevel, std::string logValue);
    static void initializeLogger(spdlog::level::level_enum logLevel, const std::string& logDir, std::size_t maxSizeMB, std::size_t backups);
    static void stopLogger();

private:
    using LogEntry = std::pair<spdlog::level::level_enum, std::string>;

    static std::vector<LogEntry> logBuffer;
    static void flushBuffer();

};

} // namespace nvd


