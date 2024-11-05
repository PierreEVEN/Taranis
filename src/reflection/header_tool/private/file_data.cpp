#include "file_data.hpp"

#include <filesystem>
#include <string>

bool FileData::Reader::starts_with(const std::string& other)
{
    skip_blank();
    for (size_t i = location; i < data->size() && i < location + other.size(); ++i)
        if ((*data)[i] != other[i - location])
            return false;
    return true;
}

bool FileData::Reader::try_consume_field(const std::string& other)
{
    if (starts_with(other))
    {
        for (size_t i = 0; i < other.size(); ++i, ++*this);
        return true;
    }
    return false;
}

std::optional<std::string> FileData::Reader::consume_next_word()
{
    skip_blank();
    std::string word;
    while ((word.empty() && std::isalpha(**this)) || (!word.empty() && std::isalnum(**this) || **this == '_'))
    {
        word += **this;
        ++*this;
    }
    return word.empty() ? std::optional<std::string>{} : word;
}

void FileData::Reader::skip_line()
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

void FileData::Reader::skip_blank()
{
    while (*this && std::isspace(**this))
        ++*this;
}

FileData::FileData(const std::filesystem::path& path) : data_stream(path)
{
    auto last       = std::filesystem::last_write_time(path);
    last_write_time = last.time_since_epoch().count();
}

std::optional<std::string> FileData::next_line()
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

FileData::Reader FileData::read()
{
    if (!finished)
        while (next_line());
    return {&loaded_data};
}