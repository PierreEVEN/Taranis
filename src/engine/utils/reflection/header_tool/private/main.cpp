#include "generator.hpp"
#include "header_parser.hpp"
#include "llp/file_data.hpp"

#include <filesystem>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 5)
    {
        std::cerr << "[Header tool] Syntax error. Expected 'header_tool <scanned_header> <target_source> <target_header> <include_path>'. Got " << argc << " params \n";
        return -1;
    }

    const std::filesystem::path scanned_header(argv[1]);
    const std::filesystem::path target_source(argv[2]);
    const std::filesystem::path target_header(argv[3]);
    const std::filesystem::path scanned_header_include_path(argv[4]);

    std::filesystem::path generated_include_path = scanned_header_include_path;
    generated_include_path = generated_include_path.replace_extension(".gen.hpp");

    auto source_header = std::make_shared<FileReader>(scanned_header);
    HeaderParser parser(source_header, generated_include_path, scanned_header);
    if (parser.get_classes().empty())
        return 0;

    if (auto include_to_add = parser.get_include_line_to_add())
    {
        std::cout << "[ Fix missing include ] : " << generated_include_path.string() << "\n";
        std::ifstream header_file(scanned_header);
        std::string   data;
        std::string   line;
        size_t        line_index = 1;
        while (std::getline(header_file, line))
        {
            if (line_index == *include_to_add)
                data += "#include \"" + generated_include_path.string() + "\"\n";
            data += line + "\n";
            line_index++;
        }
        header_file.close();
        std::ofstream output(scanned_header);
        output << data;
        output.close();
    }

    Generator generator(parser);
    generator.generate(source_header->timestamp(), target_source, target_header, scanned_header_include_path, generated_include_path);
    return 0;
}