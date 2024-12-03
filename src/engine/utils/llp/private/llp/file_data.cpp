#include "llp/file_data.hpp"

#include <filesystem>
#include <iostream>
#include <string>

bool TextReader::starts_with(const std::string& other)
{
    skip_blank();
    for (size_t i = location; i < data->size() && i < location + other.size(); ++i)
        if ((*data)[i] != other[i - location])
            return false;
    return true;
}

bool TextReader::try_consume_field(const std::string& other)
{
    if (starts_with(other))
    {
        for (size_t i = 0; i < other.size(); ++i, ++*this)
            ;
        return true;
    }
    return false;
}

std::optional<std::string> TextReader::consume_next_word()
{
    skip_blank();
    std::string word;
    while (*this && ((word.empty() && std::isalpha(**this)) || (!word.empty() && std::isalnum(**this) || **this == '_')))
    {
        word += **this;
        ++*this;
    }
    return word.empty() ? std::optional<std::string>{} : word;
}

void TextReader::skip_line()
{
    while (*this)
    {
        if (**this == '\n' || **this == '\r')
            return;
        ++*this;
    }
    if (*this && **this == '\n' || **this == '\r')
        ++*this;
}

void TextReader::skip_blank()
{
    while (*this && (static_cast<int>(**this) < 0 || std::isspace(**this)))
        ++*this;
}

TextReader FileReader::read()
{
    if (!finished)
        while (next_line())
            ;
    return TextReader{loaded_data};
}

FileReader::FileReader(const std::filesystem::path& path) : data_stream(path)
{
    auto last       = std::filesystem::last_write_time(path);
    last_write_time = last.time_since_epoch().count();
}

std::optional<std::string> FileReader::next_line()
{
    std::string line;
    if (std::getline(data_stream, line))
    {
        loaded_data += line + "\n";
        return line;
    }
    data_stream.close();
    finished = true;
    return {};
}