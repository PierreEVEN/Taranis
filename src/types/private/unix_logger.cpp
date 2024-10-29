
#include "logger.hpp"

#if OS_LINUX

#include <iostream>
#include <mutex>
#include <sstream>
#include <vector>
#include <thread>

static std::vector<uint8_t> allowed_thread_colors = {
        1, 2, 3, 4, 5, 6, 7, 8, 9,
        10, 11, 12, 13, 14, 15, 22, 23, 24, 25, 26, 27, 28, 29,
        30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
        45, 46, 47, 48, 49, 50, 51, 58, 59, 60, 61, 62, 63, 64, 65, 66,
        67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87,
        94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108,
        109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 128, 129, 130,
        131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153,154, 155,
        156, 157, 158, 159, 163, 164, 165, 166, 167, 168, 169, 170, 171, 173, 174, 175, 176, 177, 178, 179, 180,
        181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200,
        201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 210,
        221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250,
        251, 252, 253, 254, 255
};

static const char* get_log_level_color(const Logger::LogType log_level)
{
	switch (log_level)
	{
	case Logger::LogType::LOG_LEVEL_VALIDATE:
		return "\033[92m";
	case Logger::LogType::LOG_LEVEL_ERROR:
		return "\033[91m";
	case Logger::LogType::LOG_LEVEL_WARNING:
		return "\033[93m";
	case Logger::LogType::LOG_LEVEL_DEBUG:
		return "\033[96m";
	case Logger::LogType::LOG_LEVEL_INFO:
		return "\033[94m";
	case Logger::LogType::LOG_LEVEL_FATAL:
		return "\033[45;30m";
	default:
		return "\033[0m";
	}
}

	void Logger::console_print(const LogItem& in_log) {
        struct tm time_str;
        static char time_buffer[80];
        auto now = time(0);
        localtime_r(&now, &time_str);
        strftime(time_buffer, sizeof(time_buffer), "%X", &time_str);

        std::cerr << get_log_level_color(in_log.log_level);
        std::cerr << stringutils::format("[%s  ", time_buffer);

        auto worker_id = static_cast<uint8_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
        auto worker_id_str = stringutils::format("~%x", std::this_thread::get_id());
        if (thread_identifier_func && thread_identifier_func() != 255) {
            worker_id_str = stringutils::format("#W%d", thread_identifier_func());
            worker_id = thread_identifier_func();

            if (thread_identifier_func && worker_id != 255) std::cerr << "\033[40;4;38;5;" << std::to_string(
                        allowed_thread_colors[worker_id % allowed_thread_colors.size()]).c_str() << 'm';
        } else {
            std::cerr << "\033[0m";
        }

        std::cerr << stringutils::format("%s", worker_id_str.c_str());
        std::cerr << "\033[0m";
        std::cerr << get_log_level_color(in_log.log_level);

        if (in_log.function_name) std::cerr
                    << stringutils::format("] [%c] % s::% d : %s", get_log_level_char(in_log.log_level), in_log.function_name, in_log.line,
                                           in_log.message.c_str());
        else std::cerr << stringutils::format("] [%c] : %s", get_log_level_char(in_log.log_level), in_log.message.c_str());

        if (in_log.file) std::cerr << stringutils::format("\n\t=>%s", in_log.file);

        std::cerr << std::endl;
        std::cerr << "\033[0m";
    }

#endif // OS_LINUX