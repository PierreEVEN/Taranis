#pragma once
#include <cstdint>
#include <fstream>

namespace Io
{

class Stream
{
public:
    enum class Mode
    {
        Input,
        Output
    };

    Stream(Mode in_mode) : mode(in_mode)
    {
    }

    constexpr static bool is_input = false;
    virtual size_t        stream_bytes(uint8_t* bytes, size_t count) = 0;

    Mode get_mode() const
    {
        return mode;
    }

private:
    Mode mode;
};

class FileStream : public Stream
{
public:
    FileStream(Mode in_mode, const std::filesystem::path& source_file) : Stream(in_mode)
    {
        switch (get_mode())
        {
        case Mode::Input:
            input_stream = std::ifstream(source_file);
            break;
        case Mode::Output:
            output_stream = std::ofstream(source_file);
            break;
        }
    }

    size_t stream_bytes(uint8_t* bytes, size_t count) override
    {
        switch (get_mode())
        {
        case Mode::Input:
        {
            auto before = input_stream.tellg();
            input_stream.read(reinterpret_cast<char*>(bytes), static_cast<std::streamsize>(count));
            return input_stream.tellg() - before;
        }
        case Mode::Output:
        {
            output_stream.write(reinterpret_cast<char*>(bytes), static_cast<std::streamsize>(count));
            return count;
        }
        }
        return 0;
    }

private:
    std::ifstream input_stream;
    std::ofstream output_stream;
};
} // namespace Io