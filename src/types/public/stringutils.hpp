#pragma once
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "simplemacros.hpp"


namespace stringutils
{
	/**
	 * Format string.
	 * stringutils::format("%d is not a string, %s is a string, 10, "this");
	 * \param format format
	 * \param ...args arguments
	 * \return string
	 */
	template<size_t N, typename... Params>
	[[nodiscard]] std::string format(const char (&format)[N], const Params... args) {
#if CXX_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#endif
		const int size = snprintf(nullptr, 0, format, args...) + 1;
		if (size <= 0) return format;
		const std::unique_ptr<char[]> buffer(new char[size]);
		snprintf(buffer.get(), size, format, args ...);	
#if CXX_CLANG
#pragma clang diagnostic pop
#endif
		return std::string(buffer.get());
	}

	template<typename... Params>
	[[nodiscard]] std::string format_insecure(const char* format, const Params... args) {
#if CXX_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#endif
		const int size = snprintf(nullptr, 0, format, args...) + 1;
		if (size <= 0) return format;
		const std::unique_ptr<char[]> buffer(new char[size]);
		snprintf(buffer.get(), size, format, args ...);
#if CXX_CLANG
#pragma clang diagnostic pop
#endif
		return std::string(buffer.get());
	}

	typedef bool(*TrimFilterFunction)(char);
	[[nodiscard]] bool default_trim_func(char chr);	
	[[nodiscard]] std::string trim(const std::string& source, const TrimFilterFunction filter = default_trim_func);

	[[nodiscard]] std::vector<std::string> split(const std::string& source, const std::vector<char>& delimiters);

	template<typename T>
	std::string array_to_string(const std::vector<T>& in_array, const std::string& separator = ", ")
	{
		if (in_array.empty()) return "";
		std::string result;
		if constexpr (std::is_base_of<std::string, T>::value) {
			result = in_array[0];
		}
		else
		{
			result += std::to_string(in_array[0]);
		}
		for (size_t i = 1; i < in_array.size(); ++i)
		{
			if constexpr ( std::is_base_of<std::string, T>::value) {
				result += separator + in_array[i];
			}
			else
			{
				result += separator + std::to_string(in_array[i]);				
			}
		}
		return result;
	}
	
}
