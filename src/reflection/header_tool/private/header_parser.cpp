#include "header_parser.hpp"

#include "file_data.hpp"

#include <filesystem>
#include <iostream>

HeaderParser::HeaderParser(const std::shared_ptr<FileData>& header_data, const std::filesystem::path& relative_path)
{
    std::string current_class_name;

    for (auto reader = header_data->read(); reader; ++reader)
    {
        // Skip /* */ comments
        if (reader.try_consume_field("/*"))
        {
            while (reader)
                if (reader.try_consume_field("*/"))
                    break;
        }

        // Skip // comments
        else if (reader.try_consume_field("//"))
            reader.skip_line();
            // Skip quotes
        else if (reader.try_consume_field("\""))
        {
            while (reader)
            {
                if (reader.try_consume_field("\\\\") || reader.try_consume_field("\\\""))
                    continue;
                if (reader.try_consume_field("\""))
                    break;
                ++reader;
            }
        }

        // Search for generated header include
        else if (reader.try_consume_field("#include"))
        {
            if (b_found_include)
            {
                std::cerr << "Generated class include is not the last included file" << reader.current_line() << ":" << reader.current_char_index() << std::endl;
                exit(-1);
            }
            if (parse_check_include(reader, relative_path))
                b_found_include = true;
            line_after_last_include = reader.current_line() + 1;
            reader.skip_line();
        }

        // Found class declaration : is it a reflected class ?
        else if (reader.try_consume_field("class"))
        {
            if (const auto class_name = reader.consume_next_word())
            {
                std::vector<std::string> parents;
                if (reader.try_consume_field(":"))
                {
                    do
                    {
                        reader.try_consume_field("public");
                        reader.try_consume_field("private");
                        reader.try_consume_field("protected");
                        if (const auto parent_name = reader.consume_next_word())
                        {
                            std::string parent_complete_name = *parent_name;
                            if (reader.try_consume_field("<"))
                            {
                                parent_complete_name += "<";
                                size_t template_level = 1;
                                do
                                {
                                    parent_complete_name += *reader;
                                    if (reader.try_consume_field("<"))
                                        template_level++;
                                    else if (reader.try_consume_field(">"))
                                        template_level--;
                                    else
                                        ++reader;
                                } while (template_level != 0 && *reader);
                            }
                            parents.emplace_back(parent_complete_name);
                        }
                    } while (reader.try_consume_field(","));
                }

                if (reader.try_consume_field("{"))
                    if (reader.try_consume_field("REFLECT_BODY()"))
                        reflected_classes.emplace_back(ReflectedClass{*class_name, reader.current_line() + (b_found_include ? 0 : 1), parents});
            }
        }

        // Warn for dangling reflect body
        else if (reader.try_consume_field("REFLECT_BODY()"))
        {
            std::cerr << "Reflected class body should follow class declaration : " << reader.current_line() << ":" << reader.current_char_index() << std::endl;
            exit(-1);
        }
    }
}

bool HeaderParser::parse_check_include(FileData::Reader& reader, const std::filesystem::path& desired_path)
{
    bool        started = false;
    std::string include;

    while (reader)
    {
        if (*reader == '"' || *reader == '<' || *reader == '>')
        {
            ++reader;
            if (started)
                break;

            started = true;
            continue;
        }
        if (started)
            include += *reader;
        ++reader;
    }

    if (!started)
        return false;

    return std::filesystem::path(include).lexically_normal() == desired_path.lexically_normal();
}