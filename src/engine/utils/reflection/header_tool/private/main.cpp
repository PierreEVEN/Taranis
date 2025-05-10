#include "generator.hpp"
#include "header_parser.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>

struct Header
{
    std::filesystem::path header_path;
    std::filesystem::path gen_cpp_path;
    std::filesystem::path gen_hpp_path;
    std::filesystem::path include_path;
};

int main(int argc, char** argv)
{
    if (argc != 5)
    {
        std::cerr << "[Header tool] Syntax error. Expected 'header_tool <header_path> <gen_cpp_path> <gen_hpp_path> <include_path>'. Got " << argc << " arguments \n";
        exit(EXIT_FAILURE);
    }

    Header header{
        .header_path = argv[1],
        .gen_cpp_path = argv[2],
        .gen_hpp_path = argv[3],
        .include_path = argv[4]
    };

    //DependParser depend_parser(header.depend_path);
    auto  last_depend_write_time = exists(header.gen_cpp_path) ? std::filesystem::last_write_time(header.gen_cpp_path) : std::filesystem::file_time_type{};
    auto  last_write_time        = std::filesystem::last_write_time(header.header_path);
    //last_write_time = std::chrono::system_clock::to_time_t(std::chrono::clock_cast<std::chrono::system_clock>(ftime));
    if (last_depend_write_time == last_write_time)
        exit(EXIT_SUCCESS);

    std::filesystem::path generated_include_path = header.include_path;
    generated_include_path                       = generated_include_path.replace_extension(".gen.hpp");

    auto source_header = std::make_shared<FileReader>(header.header_path);
    source_header->read();
    HeaderParser parser(source_header->raw_stream(), generated_include_path, header.header_path);
    if (parser.get_classes().empty() && parser.get_enums().empty())
        exit(EXIT_SUCCESS);

    if (auto include_to_add = parser.get_include_line_to_add())
    {
        std::cout << "[ Fix missing include ] : " << generated_include_path.string() << "\n";
        std::ifstream header_file(header.header_path);
        std::string   data;
        std::string   line;
        size_t        line_index = 1;
        while (std::getline(header_file, line))
        {
            if (line_index == *include_to_add)
            {
                data += "#include \"" + generated_include_path.string() + "\"\n";
            }
            data += line + "\n";
            line_index++;
        }
        header_file.close();
        std::ofstream output(header.header_path);
        output << data;
        output.close();
    }

    Generator generator(parser);
    generator.generate(source_header->timestamp(), header.gen_cpp_path, header.gen_hpp_path, header.include_path, generated_include_path);

    std::filesystem::last_write_time(header.gen_cpp_path, std::filesystem::last_write_time(header.header_path));
    return 0;
}