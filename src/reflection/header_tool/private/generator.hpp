#pragma once
#include <fstream>

namespace std::filesystem
{
class path;
}

class HeaderParser;

class Generator
{
public:

    struct Writer
    {
        Writer(const std::filesystem::path& file);

        void write_line(const std::string& line);
        void new_line(uint32_t num = 1);
        void indent(uint32_t num = 1);
        void unindent(uint32_t num = 1);


    private:
        void make_indent();

        uint32_t      indentation = 0;
        std::ofstream fs;
    };



    Generator(HeaderParser& parser);

    void generate(size_t timestamp, const std::filesystem::path& source, const std::filesystem::path& header, const std::filesystem::path& base_header_path,
                  const std::filesystem::path& generated_header_include_path) const;

  private:





    HeaderParser* parser;
};