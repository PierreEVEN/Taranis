

#include "logger.hpp"

#include <filesystem>
#include <mutex>
#include <thread>

#include "stringutils.hpp"

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
	if (in_log.log_level & enabled_logs) {

		std::lock_guard<std::mutex> lock(logger_lock);

		if (log_function_override) log_function_override(in_log);
		else {
			file_print(in_log);
			console_print(in_log);
		}
	}
}

void Logger::set_log_file(const std::string& file)
{
	const auto log_folder = std::filesystem::path(file).parent_path();
	if (!exists(log_folder)) create_directories(log_folder);

	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
#if OS_WINDOWS
	localtime_s(&tstruct, &now);
#else
	localtime_r(&now, &tstruct);
#endif
	strftime(buf, sizeof(buf), "%Y-%m-%d.%H.%M.%S", &tstruct);

	if (log_file && *log_file) log_file->close();
	log_file = std::make_unique<std::ofstream>(stringutils::format_insecure(file.c_str(), buf));
}

void Logger::file_print(const LogItem& in_log)
{
	if (!log_file && !log_file.get()) return;

	struct tm time_str;
	static char time_buffer[80];
	auto now = time(0);
#if OS_WINDOWS
    localtime_s(&time_str, &now);
#else
    localtime_r(&now, &time_str);
#endif
	strftime(time_buffer, sizeof(time_buffer), "%X", &time_str);


	auto worker_id = static_cast<uint8_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
	auto worker_id_str = stringutils::format("~%x", std::this_thread::get_id());
	if (thread_identifier_func && thread_identifier_func() != 255)
	{
		worker_id_str = stringutils::format("#W%d", thread_identifier_func());
		worker_id = thread_identifier_func();
	}

	*log_file << stringutils::format("[%s %s] [%c] % s::% d : %s\n", time_buffer, worker_id_str.c_str(), get_log_level_char(in_log.log_level), in_log.function_name, in_log.line, in_log.message.c_str());
	log_file->flush();
}
