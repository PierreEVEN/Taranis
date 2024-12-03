#pragma once

#include "simplemacros.hpp"

#if __has_include("filesystem")
#include <filesystem>
#else
#include <experimental/filesystem>
#endif
#include <fstream>
#include <mutex>

#if CXX_MSVC
#if CXX_LEVEL_DEBUG
#define __SIMPLE_LOG(format_str, log_level, ...)   Logger::get().print(Logger::LogItem(log_level, std::format(format_str, __VA_ARGS__), ##__FUNCTION__, __LINE__))
#define __ADVANCED_LOG(format_str, log_level, ...) Logger::get().print(Logger::LogItem(log_level, std::format(format_str, __VA_ARGS__), ##__FUNCTION__, __LINE__, ##__FILE__))
#define __LOG_FULL_ASSERT(format_str, log_level, ...)                                                                                \
    {                                                                                                                                \
        Logger::get().print(Logger::LogItem(log_level, std::format(format_str, __VA_ARGS__), ##__FUNCTION__, __LINE__, ##__FILE__)); \
        __debugbreak();                                                                                                              \
        exit(EXIT_FAILURE);                                                                                                          \
    }
#else
#define __SIMPLE_LOG(format_str, log_level, ...)   Logger::get().print(Logger::LogItem(log_level, std::format(format_str, __VA_ARGS__)))
#define __ADVANCED_LOG(format_str, log_level, ...) Logger::get().print(Logger::LogItem(log_level, std::format(format_str, __VA_ARGS__), ##__FUNCTION__, __LINE__, ##__FILE__))
#define __LOG_FULL_ASSERT(format_str, log_level, ...)                                                                                \
    {                                                                                                                                \
        Logger::get().print(Logger::LogItem(log_level, std::format(format_str, __VA_ARGS__), ##__FUNCTION__, __LINE__, ##__FILE__)); \
        exit(EXIT_FAILURE);                                                                                                          \
    }
#endif
#elif CXX_GCC
#define __SIMPLE_LOG(format_str, log_level, ...)   Logger::get().print(Logger::LogItem(log_level, std::format(format_str, ##__VA_ARGS__), __FUNCTION__, __LINE__))
#define __ADVANCED_LOG(format_str, log_level, ...) Logger::get().print(Logger::LogItem(log_level, std::format(format_str, ##__VA_ARGS__), __FUNCTION__, __LINE__, __FILE__))
#define __LOG_FULL_ASSERT(format_str, log_level, ...)                                                                              \
    {                                                                                                                              \
        Logger::get().print(Logger::LogItem(log_level, std::format(format_str, ##__VA_ARGS__), __FUNCTION__, __LINE__, __FILE__)); \
        exit(EXIT_FAILURE);                                                                                                        \
    }
#elif CXX_CLANG
#define __SIMPLE_LOG(format_str, log_level, ...)   Logger::get().print(Logger::LogItem(log_level, std::format(format_str __VA_OPT__(, ) __VA_ARGS__)))
#define __ADVANCED_LOG(format_str, log_level, ...) Logger::get().print(Logger::LogItem(log_level, std::format(format_str __VA_OPT__(, ) __VA_ARGS__), __FUNCTION__, __LINE__, __FILE__))
#define __LOG_FULL_ASSERT(format_str, log_level, ...)                                                                                          \
    {                                                                                                                                          \
        Logger::get().print(Logger::LogItem(log_level, std::format(format_str __VA_OPT__(, ) __VA_ARGS__), __FUNCTION__, __LINE__, __FILE__)); \
        exit(EXIT_FAILURE);                                                                                                                    \
    }
#endif

#if CXX_MSVC
#define LOG_FATAL(format_str, ...)    __LOG_FULL_ASSERT(format_str, Logger::get().LogType::LOG_LEVEL_FATAL, __VA_ARGS__)
#define LOG_VALIDATE(format_str, ...) __SIMPLE_LOG(format_str, Logger::get().LogType::LOG_LEVEL_VALIDATE, __VA_ARGS__)
#define LOG_ERROR(format_str, ...)    __ADVANCED_LOG(format_str, Logger::get().LogType::LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOG_WARNING(format_str, ...)  __SIMPLE_LOG(format_str, Logger::get().LogType::LOG_LEVEL_WARNING, __VA_ARGS__)
#define LOG_INFO(format_str, ...)     __SIMPLE_LOG(format_str, Logger::get().LogType::LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_TRACE(format_str, ...)    __SIMPLE_LOG(format_str, Logger::get().LogType::LOG_LEVEL_TRACE, __VA_ARGS__)
#define LOG_DEBUG(format_str, ...)    __SIMPLE_LOG(format_str, Logger::get().LogType::LOG_LEVEL_DEBUG, __VA_ARGS__)
#elif CXX_GCC
#define LOG_FATAL(format_str, ...)    __LOG_FULL_ASSERT(format_str, Logger::LogType::LOG_LEVEL_FATAL, ##__VA_ARGS__)
#define LOG_VALIDATE(format_str, ...) __SIMPLE_LOG(format_str, Logger::LogType::LOG_LEVEL_VALIDATE, ##__VA_ARGS__)
#define LOG_ERROR(format_str, ...)    __ADVANCED_LOG(format_str, Logger::LogType::LOG_LEVEL_ERROR, ##__VA_ARGS__)
#define LOG_WARNING(format_str, ...)  __SIMPLE_LOG(format_str, Logger::LogType::LOG_LEVEL_WARNING, ##__VA_ARGS__)
#define LOG_INFO(format_str, ...)     __SIMPLE_LOG(format_str, Logger::LogType::LOG_LEVEL_INFO, ##__VA_ARGS__)
#define LOG_TRACE(format_str, ...)    __SIMPLE_LOG(format_str, Logger::LogType::LOG_LEVEL_TRACE, ##__VA_ARGS__)
#define LOG_DEBUG(format_str, ...)    __SIMPLE_LOG(format_str, Logger::LogType::LOG_LEVEL_DEBUG, ##__VA_ARGS__)
#elif CXX_CLANG
#define LOG_FATAL(format_str, ...)    __LOG_FULL_ASSERT(format_str, Logger::LogType::LOG_LEVEL_FATAL __VA_OPT__(, ) __VA_ARGS__)
#define LOG_VALIDATE(format_str, ...) __SIMPLE_LOG(format_str, Logger::LogType::LOG_LEVEL_VALIDATE __VA_OPT__(, ) __VA_ARGS__)
#define LOG_ERROR(format_str, ...)    __ADVANCED_LOG(format_str, Logger::LogType::LOG_LEVEL_ERROR __VA_OPT__(, ) __VA_ARGS__)
#define LOG_WARNING(format_str, ...)  __SIMPLE_LOG(format_str, Logger::LogType::LOG_LEVEL_WARNING __VA_OPT__(, ) __VA_ARGS__)
#define LOG_INFO(format_str, ...)     __SIMPLE_LOG(format_str, Logger::LogType::LOG_LEVEL_INFO __VA_OPT__(, ) __VA_ARGS__)
#define LOG_TRACE(format_str, ...)    __SIMPLE_LOG(format_str, Logger::LogType::LOG_LEVEL_TRACE __VA_OPT__(, ) __VA_ARGS__)
#define LOG_DEBUG(format_str, ...)    __SIMPLE_LOG(format_str, Logger::LogType::LOG_LEVEL_DEBUG __VA_OPT__(, ) __VA_ARGS__)
#endif

class Logger
{
  public:
    enum LogType
    {
        LOG_LEVEL_FATAL    = 1 << 0,
        LOG_LEVEL_VALIDATE = 1 << 1,
        LOG_LEVEL_ERROR    = 1 << 2,
        LOG_LEVEL_WARNING  = 1 << 3,
        LOG_LEVEL_INFO     = 1 << 4,
        LOG_LEVEL_DEBUG    = 1 << 5,
        LOG_LEVEL_TRACE    = 1 << 6,
    };

    struct LogItem
    {
        LogItem(LogType in_log_level, const std::string& in_message, const char* in_function = nullptr, size_t in_line = 0, const char* in_file = nullptr)
            : log_level(in_log_level), message(in_message), function_name(in_function), line(in_line), file(in_file)
        {
        }

        LogType     log_level;
        std::string message;
        const char* function_name;
        size_t      line;
        const char* file;
    };

    using LogFunctionOverrideType      = void (*)(const LogItem&);
    using ThreadIdentifierFunctionType = uint8_t (*)();

    /**
     * Threads can be identified with a custom ID.
     * return 255 if thread is unknown.
     *
     * ie : logger_set_thread_identifier_func([] () -> uint8_t { return is_worker_thread() ? return get_worker_thread_id() : 255 });
     */
    void set_thread_identifier(ThreadIdentifierFunctionType func)
    {
        thread_identifier_func = func;
    }

    void set_log_function_override(LogFunctionOverrideType in_function)
    {
        log_function_override = in_function;
    }

    void set_log_file(const std::string& file);

    /**
     * Set and get what kind of logs are displayed.
     */
    void enable_logs(uint32_t log_level);
    void disable_logs(uint32_t log_level);

    /**
     * Append a log to the list of logs to display
     */
    void print(const LogItem& in_log);

    static Logger& get()
    {
        if (!logger_instance)
            logger_instance = std::make_unique<Logger>();
        return *logger_instance.get();
    }

    /**
     * print a log to terminal stdout (with colors)
     */
    void console_print(const LogItem& in_log);
    void file_print(const LogItem& in_log);

  private:
    ThreadIdentifierFunctionType thread_identifier_func = nullptr;
    LogFunctionOverrideType      log_function_override  = nullptr;

    /**
     * Get log level as char
     */
    [[nodiscard]] char get_log_level_char(const LogType log_level);

    /**
     * Thread safety
     */
    std::mutex logger_lock;

    /**
     * Output file
     */
    std::unique_ptr<std::ofstream> log_file = nullptr;

    /**
     * List of enabled logs
     */
    uint32_t enabled_logs = LOG_LEVEL_FATAL | LOG_LEVEL_VALIDATE | LOG_LEVEL_ERROR | LOG_LEVEL_WARNING;

    /**
     * Logger singleton instance
     */
    inline static std::unique_ptr<Logger> logger_instance = nullptr;
};