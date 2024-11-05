#pragma once
#include <algorithm>
#include <string>

// trim from start (in place)
inline std::string ltrim(const std::string& s)
{
    std::string res = s;
    res.erase(std::ranges::find_if_not(res, std::isspace), res.end());
    return res;
}

// trim from end
inline std::string rtrim(const std::string& s)
{
    std::string res = s;
    res.erase(std::find_if_not(res.rbegin(), res.rend(), std::isspace), res.end());
    return res;
}

// trim from both ends
inline std::string trim(const std::string& s)
{
    return ltrim(rtrim(s));
}