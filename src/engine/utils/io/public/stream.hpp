#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>

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
    virtual ~Stream() = default;

    virtual size_t        stream_bytes(uint8_t* bytes, size_t count) = 0;
    virtual void          flush()                                    = 0;

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
        {
            input_stream = std::ifstream(source_file);
            if (!input_stream.is_open())
                std::cerr << "Cannot open " << source_file.generic_string() << " for reading\n";
            break;
        }
        case Mode::Output:
        {
            output_stream = std::ofstream(source_file);
            if (!output_stream.is_open())
                std::cerr << "Cannot open " << source_file.generic_string() << " for writing\n";
            break;
        }
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

    void flush() override
    {
        if (get_mode() == Mode::Output)
            output_stream.flush();
    }

    virtual ~FileStream()
    {
        switch (get_mode())
        {
        case Mode::Input:
        {
            input_stream.close();
            break;
        }
        case Mode::Output:
        {
            output_stream.close();
            break;
        }
        }
    }

private:
    std::ifstream input_stream;
    std::ofstream output_stream;
};
} // namespace Io