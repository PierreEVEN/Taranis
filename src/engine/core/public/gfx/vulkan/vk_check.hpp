#pragma once

#include "stringutils.hpp"
#include "logger.hpp"
#include "magic_enum.h"

#define VK_CHECK(condition, text, ...)                                                            \
    if ((condition) != VK_SUCCESS)                                                                \
    {                                                                                             \
        LOG_FATAL("{} : {}", magic_enum::enum_name(static_cast<VkResult>(condition)).data(), stringutils::format(text, __VA_ARGS__).c_str()); \
    }

template <class T>
class Result
{
public:
	~Result()
	{
	}

	T get()
	{
		if (!valid)
			LOG_FATAL("Reading invalid result value : {}", result_error.c_str())
		return value;
	}

	std::string error()
	{
		if (valid)
			return result_error;
		return "";
	}

	bool is_ok() const
	{
		return valid;
	}

	static Result Ok(T value)
	{
		return Result(true, std::move(value));
	}

	static Result Error(std::string message)
	{
		return Result(std::move(message));
	}

	explicit operator bool() const { return valid; }

private:
	Result(std::string error_message): result_error(std::move(error_message))
	{
	}

	Result(bool success, T value) : valid(success), value(std::move(value))
	{
	}

	bool valid = false;

	union
	{
		std::string result_error;
		T value;
	};
};
