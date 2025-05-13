

#include "logger.hpp"

#include <filesystem>
#include <mutex>
#include <thread>

char Logger::get_log_level_char(const LogType log_level)
{
    switch (log_level)
    {
    case LOG_LEVEL_VALIDATE:
        return 'V';
    case LOG_LEVEL_ERROR:
        return 'E';
    case LOG_LEVEL_WARNING:
        return 'W';
    case LOG_LEVEL_INFO:
        return 'I';
    case LOG_LEVEL_TRACE:
        return 'T';
    case LOG_LEVEL_DEBUG:
        return 'D';
    case LOG_LEVEL_FATAL:
        return 'F';
    }
    return 'X';
}

void Logger::enable_logs(uint32_t log_level)
{
    enabled_logs |= log_level;
}

void Logger::disable_logs(uint32_t log_level)
{
    enabled_logs &= ~log_level;
}

void Logger::print(const LogItem& in_log)
{
    if (in_log.log_level & enabled_logs)
    {

        std::lock_guard<std::mutex> lock(logger_lock);

        if (log_function_override)
            log_function_override(in_log);
        else
        {
            file_print(in_log);
            console_print(in_log);
        }
    }
}

void Logger::file_print(const LogItem& in_log)
{
    if (!log_file && !log_file.get())
        return;

    struct tm   time_str;
    static char time_buffer[80];
    auto        now = time(0);
#if OS_WINDOWS
    localtime_s(&time_str, &now);
#else
    localtime_r(&now, &time_str);
#endif
    strftime(time_buffer, sizeof(time_buffer), "{}", &time_str);

    std::stringstream thread_id;
    thread_id << std::this_thread::get_id();
    auto worker_id_str = std::format("~{}", thread_id.str());
    if (thread_identifier_func && thread_identifier_func() != 255)
    {
        worker_id_str = std::format("#W{}", thread_identifier_func());
    }

    *log_file << std::format("[{} {}] [{}] {}::{} : {}\n", time_buffer, worker_id_str.c_str(), get_log_level_char(in_log.log_level), in_log.function_name, in_log.line, in_log.message.c_str());
    log_file->flush();
}
