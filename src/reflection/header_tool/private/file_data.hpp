#pragma once
#include <fstream>
#include <iostream>
#include <optional>

namespace std::filesystem
{
class path;
}

class FileData
{

  public:
    struct Reader
    {
        friend FileData;

        Reader& operator++()
        {
            if (*this && **this == '\n')
            {
                current_char_offset = 0;
                current_line_index++;
            }
            else
                current_char_offset++;

            location++;

            return *this;
        }

        operator bool() const
        {
            return location >= 0 && location < static_cast<int64_t>(data->size());
        }

        char operator*() const
        {
            return (*data)[location];
        }

        size_t current_line() const
        {
            return current_line_index;
        }

        size_t current_char_index() const
        {
            return current_char_offset;
        }

        bool                       starts_with(const std::string& other);
        bool                       try_consume_field(const std::string& other);
        std::optional<std::string> consume_next_word();
        void                       skip_line();
        void                       skip_blank();

      private:
        Reader(std::string* in_data) : data(in_data)
        {
        }

        size_t       current_line_index  = 1;
        size_t       current_char_offset = 0;
        int64_t      location            = 0;
        std::string* data                = nullptr;
    };

    FileData(const std::filesystem::path& path);

    std::optional<std::string> next_line();

    Reader read();
    size_t timestamp() const
    {
        return last_write_time;
    }

  private:
    size_t      last_write_time = 0;
    std::string loaded_data;
    bool        finished = false;

    std::ifstream data_stream;
};