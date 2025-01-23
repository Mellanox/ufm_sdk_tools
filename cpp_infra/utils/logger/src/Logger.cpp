#include "utils/logger/Logger.h"

#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

static const size_t LOG_QUEUE_SIZE = 10000;
static const size_t LOG_THREADS = 1;

namespace nvd {

std::vector<Log::LogEntry> Log::logBuffer;

/*
 * logBuffer shall be used for logging only before logger got initialized.
 * After logger initialization buffer is flushed.
 */
void Log::addLogEntryToBuffer(spdlog::level::level_enum logLevel, std::string logValue) 
{
    logBuffer.push_back(std::pair(logLevel, logValue));
}

void Log::flushBuffer() 
{
    for (auto v:logBuffer) 
    {
        const auto& [logLevel, logValue] = v;
        NVD_LOG(logLevel, logValue);
    }
    logBuffer.clear();
}

// todo - add TOPIC / Comp  -> e.g. "UFM"

void Log::initializeLogger(spdlog::level::level_enum logLevel, const std::string& logDir, std::size_t maxSizeMB, std::size_t backups)
{
    try
    {
        spdlog::init_thread_pool(LOG_QUEUE_SIZE, LOG_THREADS); // queue with 10K log items and 1 thread

        auto console_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logDir + "/nvd_console.log",
                                                                                1024*1024*maxSizeMB, backups);
        const spdlog::sinks_init_list sink_list = { console_sink, file_sink };
        
        //block new log items if queue with old items is full
        auto logger = std::make_shared<spdlog::async_logger>("NVD", sink_list.begin(), sink_list.end(),
                                                            spdlog::thread_pool(), spdlog::async_overflow_policy::block);
        logger->set_level(logLevel);
        
        // Log entry patern: [Date_and_time] [logger_name] [colored_log_level] [file:line] log_value
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [%s:%#] %v");

        // todo - check how to add user logger
        spdlog::set_default_logger(logger);
    }    
    catch(const spdlog::spdlog_ex& error) 
    {
        //set log level to default console logger only, if not succeeded with file
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [NVD] [%^%l%$] [%s:%#] %v");
        spdlog::set_level(logLevel);
        LOGERROR("File log initialization failed: {}. Will use console log only.", error.what());
    }

    flushBuffer();
}

void Log::stopLogger() 
{
    spdlog::drop_all();
}


} // namespace