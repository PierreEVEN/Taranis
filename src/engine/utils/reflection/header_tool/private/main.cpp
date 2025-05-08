#include "generator.hpp"
#include "header_parser.hpp"
#include "llp/file_data.hpp"

#include <filesystem>
#include <iostream>

#include "depend_parser.h"

int main(int argc, char **argv) {
    if (argc != 7) {
        std::cerr <<
                "[Header tool] Syntax error. Expected 'header_tool <scanned_header> <target_source> <target_header> <include_path> <depend_path> <object_path>'. Got "
                << argc << " params \n";
        return -1;
    }

    const std::filesystem::path scanned_header(argv[1]);
    const std::filesystem::path target_source(argv[2]);
    const std::filesystem::path target_header(argv[3]);
    const std::filesystem::path scanned_header_include_path(argv[4]);
    const std::filesystem::path depend_file_path(argv[5]);
    const std::filesystem::path gen_object_path(argv[6]);
    std::filesystem::path generated_include_path = scanned_header_include_path;
    generated_include_path = generated_include_path.replace_extension(".gen.hpp");

    DependParser depend_parser(depend_file_path);
    std::time_t last_depend_write_time = depend_parser.get_last_write_time();

    std::time_t last_write_time = 0;
    if (exists(gen_object_path)) {
        auto ftime = std::filesystem::last_write_time(gen_object_path);
        last_write_time = std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(ftime));
    }

    if (last_depend_write_time != 0 && last_depend_write_time <= last_write_time) {
        std::cout << scanned_header << " UP TO DATE : " << last_depend_write_time << " vs Source : " << last_write_time
                << "\n";
        return 0;
    }
    auto source_header = std::make_shared<FileReader>(scanned_header);
    source_header->read();
    HeaderParser parser(source_header->raw_stream(), generated_include_path, scanned_header);
    if (parser.get_classes().empty() && parser.get_enums().empty())
        return 0;
 std::cout << scanned_header << " MODIFY : " << last_depend_write_time << " vs Source : " << last_write_time << "\n";

    if (auto include_to_add = parser.get_include_line_to_add()) {
        std::cout << "[ Fix missing include ] : " << generated_include_path.string() << "\n";
        std::ifstream header_file(scanned_header);
        std::string data;
        std::string line;
        size_t line_index = 1;
        while (std::getline(header_file, line)) {
            if (line_index == *include_to_add) {
                data += "#include \"" + generated_include_path.string() + "\"\n";
            }
            data += line + "\n";
            line_index++;
        }
        header_file.close();
        std::ofstream output(scanned_header);
        output << data;
        output.close();
    }

    Generator generator(parser);
    generator.generate(source_header->timestamp(), target_source, target_header, scanned_header_include_path,
                       generated_include_path);
    return 0;
}
