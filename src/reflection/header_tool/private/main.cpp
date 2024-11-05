#include "file_data.hpp"
#include "generator.hpp"
#include "header_parser.hpp"

#include <filesystem>
#include <iostream>


int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Header tool require a search directory and an output directory" << std::endl;
        return -1;
    }

    std::filesystem::path search_dir(argv[1]);
    std::filesystem::path output_dir(argv[2]);

    bool b_started = false;
    auto start = std::chrono::steady_clock::now();

    for (auto const& dir_entry : std::filesystem::recursive_directory_iterator{search_dir})
    {
        if (dir_entry.is_regular_file())
        {
            std::filesystem::path fixed_search_dir = search_dir;
            std::filesystem::path fixed_output_dir = output_dir;

            auto relative_entry = relative(dir_entry.path(), search_dir);
            if (relative_entry.string().starts_with("public"))
            {
                fixed_search_dir = search_dir / "public";
                fixed_output_dir = output_dir / "private";
            }
            else if (relative_entry.string().starts_with("private"))
            {
                fixed_search_dir = search_dir / "private";
                fixed_output_dir = output_dir / "private";
            }

            const auto extension = dir_entry.path().extension();
            if (extension == ".hpp" || extension == ".h")
            {
                auto source_header = std::make_shared<FileData>(dir_entry.path());

                auto generated_header = std::filesystem::path(output_dir) / "public" / relative(dir_entry.path(), fixed_search_dir);
                auto generated_source = std::filesystem::path(output_dir) / "private" / relative(dir_entry.path(), fixed_search_dir);

                generated_header = generated_header.replace_extension(".gen.hpp");
                generated_source = generated_source.replace_extension(".gen.cpp");

                if (exists(generated_header))
                {
                    FileData target_data(generated_header);
                    if (auto first_line = target_data.next_line())
                    {
                        if (first_line->starts_with("/// VERSION : "))
                        {
                            size_t read_chrs       = 0;
                            size_t last_write_time = std::stoull(first_line->substr(14, std::string::npos), &read_chrs);
                            if (read_chrs != 0 && source_header->timestamp() == last_write_time)
                            {
                                // The file was not modified
                                continue;
                            }
                        }
                    }
                }

                if (!b_started)
                    std::cout << "[ building reflection data for " << search_dir.filename().string() << "]" << std::endl;
                b_started = true;

                std::filesystem::path required_include = relative(dir_entry.path(), fixed_search_dir).replace_extension(".gen.hpp");
                HeaderParser          parser(source_header, required_include);
                if (auto include_to_add = parser.get_include_line_to_add())
                {
                    std::ifstream header_file(dir_entry.path());
                    std::string   data;
                    std::string   line;
                    size_t        line_index = 1;
                    while (std::getline(header_file, line))
                    {
                        if (line_index == *include_to_add)
                            data += "#include \"" + required_include.string() + "\"\n";
                        data += line + "\n";
                        line_index++;
                    }
                    header_file.close();
                    std::ofstream output(dir_entry.path());
                    output << data;
                    output.close();
                }

                Generator generator(parser);
                generator.generate(source_header->timestamp(), generated_source, generated_header, relative(dir_entry.path(), fixed_search_dir), relative(generated_header, output_dir / "public"));

                std::cout << "\t- " << relative(dir_entry.path(), search_dir).replace_extension(".gen.cpp").string() << std::endl;
            }
        }
    }
    if (b_started)
    {
        std::cout << "[ generated reflection data in " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count() / 1000.0 << "ms]" << std::endl;

    }

    return 0;
}