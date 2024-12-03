
#include "stringutils.hpp"
#include <algorithm>

bool stringutils::default_trim_func(char chr)
{
    return chr == ' ' || chr == '\t' || chr == '\n' || chr == '\r';
}

std::string stringutils::trim(const std::string& source, const stringutils::TrimFilterFunction filter)
{
    if (source.empty())
        return "";
    assert(filter);
    size_t start = 0, end = source.size();

    while (filter(source[start]))
    {
        if (start == end - 1)
            return "";
        start++;
    }

    while (filter(source[--end]))
    {
    }
    return std::string(source.begin() + start, source.begin() + end + 1);
}

std::vector<std::string> stringutils::split(const std::string& source, const std::vector<char>& delimiters)

{
    std::vector<std::string> result;
    std::string              left_side;

    for (const auto& chr : source)
    {
        if (std::find(delimiters.begin(), delimiters.end(), chr) != delimiters.end())
        {
            if (!left_side.empty())
            {
                result.emplace_back(left_side);
                left_side.clear();
            }
        }
        else
        {
            left_side += chr;
        }
    }
    result.emplace_back(left_side);
    return result;
}
