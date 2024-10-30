
#include <fstream>

#include "logger.hpp"

#if OS_WINDOWS

#include <iostream>
#include <mutex>
#include <vector>
#include <thread>
#include <filesystem>

#include <Windows.h>

HANDLE h_console_out = GetStdHandle(STD_OUTPUT_HANDLE);

static std::vector<uint8_t> allowed_thread_colors = {
	2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	18, 19, 20, 22, 23, 24, 26, 27, 28, 30, 31,
	32, 33, 37, 38, 39, 43, 44, 45, 46, 47,
	48, 49, 52, 54, 55, 58, 59, 60, 61, 62, 63,
	64, 65, 66, 67, 70, 71, 73, 74, 75, 76, 78, 79,
	80, 81, 82, 83, 84, 86, 87, 90, 91, 92, 94, 95,
	96, 98, 99, 100, 101, 104, 105, 106, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 120, 121, 122, 123, 125, 127,
	129, 132, 133, 134 ,135 ,138, 139, 140, 142, 143,
	144, 145, 148, 149, 150, 151, 154, 155, 156, 157, 158, 159,
	160, 161, 164, 165, 166, 167, 162, 174, 175,
	176, 177, 178, 179, 180 ,181, 184, 185, 186, 189, 190, 191,
	192, 193, 194, 197, 201, 202, 206, 207,
	208, 209, 210, 211, 214, 215, 220, 222, 223,
	224, 225, 226, 227, 228, 229, 232, 233, 234, 235, 236, 237,
	240, 241, 242, 243 ,244 ,245, 248, 249, 250, 252, 253
};

enum Win_ConsoleColor {
	CONSOLE_FG_COLOR_BLACK = 0,
	CONSOLE_FG_COLOR_BLUE = 1,
	CONSOLE_FG_COLOR_GREEN = 2,
	CONSOLE_FG_COLOR_RED = 4,
	CONSOLE_FG_COLOR_LIGHT = 8,
	CONSOLE_FG_COLOR_LIGHT_RED = CONSOLE_FG_COLOR_RED | CONSOLE_FG_COLOR_LIGHT,
	CONSOLE_FG_COLOR_WHITE = CONSOLE_FG_COLOR_RED | CONSOLE_FG_COLOR_GREEN | CONSOLE_FG_COLOR_BLUE,
	CONSOLE_FG_COLOR_ORANGE = CONSOLE_FG_COLOR_RED | CONSOLE_FG_COLOR_GREEN,
	CONSOLE_FG_COLOR_VIOLET = CONSOLE_FG_COLOR_RED | CONSOLE_FG_COLOR_BLUE,
	CONSOLE_FG_COLOR_LIGHT_BLUE = CONSOLE_FG_COLOR_GREEN | CONSOLE_FG_COLOR_BLUE,
	CONSOLE_FG_COLOR_CYAN = CONSOLE_FG_COLOR_GREEN | CONSOLE_FG_COLOR_BLUE | CONSOLE_FG_COLOR_LIGHT,
	CONSOLE_BG_COLOR_BLUE = 16,
	CONSOLE_BG_COLOR_GREEN = 32,
	CONSOLE_BG_COLOR_RED = 64,
	CONSOLE_BG_COLOR_LIGHT = 128,
	CONSOLE_DEFAULT = CONSOLE_FG_COLOR_WHITE,
	CONSOLE_VALIDATE = CONSOLE_FG_COLOR_GREEN,
	CONSOLE_DISPLAY = CONSOLE_FG_COLOR_LIGHT_BLUE,
	CONSOLE_WARNING = CONSOLE_FG_COLOR_ORANGE,
	CONSOLE_FAIL = CONSOLE_FG_COLOR_LIGHT_RED,
	CONSOLE_ASSERT = CONSOLE_FG_COLOR_VIOLET | CONSOLE_BG_COLOR_RED | CONSOLE_BG_COLOR_GREEN | CONSOLE_BG_COLOR_LIGHT,
	CONSOLE_TRACE = CONSOLE_FG_COLOR_WHITE
};

static WORD get_log_level_color(const Logger::LogType log_level)
{
	switch (log_level)
	{
	case Logger::LogType::LOG_LEVEL_VALIDATE:
		return CONSOLE_VALIDATE;
	case Logger::LogType::LOG_LEVEL_ERROR:
		return CONSOLE_FAIL;
	case Logger::LogType::LOG_LEVEL_WARNING:
		return CONSOLE_WARNING;
	case Logger::LogType::LOG_LEVEL_DEBUG:
		return CONSOLE_DISPLAY;
	case Logger::LogType::LOG_LEVEL_INFO:
		return CONSOLE_FG_COLOR_CYAN;
	case Logger::LogType::LOG_LEVEL_FATAL:
		return CONSOLE_ASSERT;
	case Logger::LogType::LOG_LEVEL_TRACE:
		return CONSOLE_TRACE;
	default:
		return CONSOLE_DEFAULT;
	}
}

void Logger::console_print(const LogItem& in_log)
{
	struct tm time_str;
	static char time_buffer[80];
	auto now = time(0);
	localtime_s(&time_str, &now);
	strftime(time_buffer, sizeof(time_buffer), "{}", &time_str);

	SetConsoleTextAttribute(h_console_out, get_log_level_color(in_log.log_level));
	std::cerr << std::format("[{}  ", time_buffer);

	std::stringstream thread_id;
	thread_id << std::this_thread::get_id();
	auto worker_id_str = std::format("~{}", thread_id.str());
	if (thread_identifier_func && thread_identifier_func() != 255)
	{
		worker_id_str = std::format("#W{}", thread_identifier_func());
		auto worker_id = thread_identifier_func();

		if (thread_identifier_func && worker_id != 255) SetConsoleTextAttribute(h_console_out, allowed_thread_colors[worker_id % allowed_thread_colors.size()]);
	}
	else
	{
		SetConsoleTextAttribute(h_console_out, CONSOLE_ASSERT);
	}

	std::cerr << std::format("{}", worker_id_str.c_str());
	SetConsoleTextAttribute(h_console_out, get_log_level_color(in_log.log_level));
	if (in_log.function_name) std::cerr << std::format("] [{}] {}::{} : {}", get_log_level_char(in_log.log_level), in_log.function_name, in_log.line, in_log.message.c_str());
	else std::cerr << std::format("] [{}] : {}", get_log_level_char(in_log.log_level), in_log.message.c_str());

	if (in_log.file) std::cerr << std::format("\n\t=>{}", in_log.file);

	std::cerr << std::endl;
	SetConsoleTextAttribute(h_console_out, CONSOLE_DEFAULT);
}

#endif // OS_WINDOWS