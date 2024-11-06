#include "header_parser.hpp"

#include "file_data.hpp"
#include "lexical_analyzer.hpp"

#include <filesystem>
#include <iostream>


HeaderParser::HeaderParser(const std::shared_ptr<FileData>& header_data, const std::filesystem::path& in_generated_header_include_path, const std::filesystem::path& in_header_path)
    : generated_header_include_path(in_generated_header_include_path), header_path(in_header_path)
{
    auto reader    = header_data->read();
    tokenized_file = Block(reader);
    parse_block(tokenized_file, {});

}

std::string HeaderParser::ReflectedClass::class_path() const
{
    std::string name;
    for (const auto& ns : context.namespace_stack)
        name += "::" + ns;
    for (const auto& class_name : context.class_stack)
        name += "::" + class_name.name;
    return name;
}

std::string HeaderParser::ReflectedClass::sanitized_class_path() const
{
    std::string name;
    for (const auto& ns : context.namespace_stack)
        name += "_" + ns;
    for (const auto& class_name : context.class_stack)
        name += "_" + class_name.name;
    return name;
}

std::vector<std::string> HeaderParser::ReflectedClass::get_parent_paths() const
{
    std::string path;
    for (const auto& ns : context.namespace_stack)
        path += "::" + ns;
    for (size_t i = 0; i < context.class_stack.size() - 1; ++i)
        path += "::" + context.class_stack[i].name;
    std::vector<std::string> parents;
    for (const auto& parent : context.class_stack.back().parents)
        parents.push_back(path + "::" + parent);
    return parents;
}

std::string HeaderParser::ReflectedClass::class_name() const
{
    return context.class_stack.back().name;
}

std::string HeaderParser::ReflectedClass::namespace_path() const
{
    std::string name;
    bool        b_is_first = true;
    for (const auto& ns : context.namespace_stack)
    {
        name += (b_is_first ? "" : "::") + ns;
        b_is_first = false;
    }
    return name;
}

void HeaderParser::parse_block(Block& block, const ParserContext& context)
{
    for (auto reader = block.read(); reader && !reader.consume<Symbol>(TokenType::Symbol, '}');)
    {
        switch (reader->type)
        {
        case TokenType::Symbol:
            reader.consume<Symbol>(TokenType::Symbol);
            break;
        case TokenType::Word:
        {
            const std::string& word = *reader.consume<Word>(TokenType::Word);
            // Class declaration
            if (word == "class")
            {
                if (auto class_name = reader.consume<Word>(TokenType::Word))
                {
                    std::vector<std::string> parents;
                    if (reader.consume<Symbol>(TokenType::Symbol, ':'))
                    {
                        do
                        {
                            // Skip fields
                            reader.consume<Word>(TokenType::Word, "public") || reader.consume<Word>(TokenType::Word, "protected") || reader.consume<Word>(TokenType::Word, "private");

                            if (auto parent_class = reader.consume<Word>(TokenType::Word))
                            {
                                std::string parent = *parent_class;

                                if (reader.consume<Symbol>(TokenType::Symbol, '<'))
                                {
                                    size_t template_level = 1;
                                    do
                                    {
                                        if (reader.consume<Symbol>(TokenType::Symbol, '<'))
                                        {
                                            parent += '<';
                                            template_level++;
                                        }
                                        else if (auto* template_str = reader.consume<Word>(TokenType::Word))
                                            parent += *template_str;
                                        else if (reader.consume<Symbol>(TokenType::Symbol, '>'))
                                        {
                                            parent += '>';
                                            template_level--;
                                        }

                                    } while (reader && template_level != 0);
                                }

                                parents.push_back(parent);
                            }
                            else
                                error("Expected class name", reader->line, reader->column);
                        } while (reader.consume<Symbol>(TokenType::Symbol, ','));
                    }
                    if (auto* class_block = reader.consume<Block>(TokenType::Block))
                        parse_block(*class_block, context.push_class(ClassDefinition{*class_name, parents}));
                }
            }
            else if (word == "namespace")
            {
                std::vector<std::string> added_namespace_stack;
                bool                     b_failed = false;
                do
                {
                    reader.consume<Word>(TokenType::Word, "::");
                    if (auto namespace_name = reader.consume<Word>(TokenType::Word))
                        added_namespace_stack.push_back(*namespace_name);
                    else
                    {
                        b_failed = true;
                        break;
                    }

                } while (reader->type != TokenType::Block);
                if (!b_failed)
                {
                    ParserContext new_context = context;
                    for (const auto& elem : added_namespace_stack)
                        new_context = new_context.push_namespace(elem);
                    if (auto* class_block = reader.consume<Block>(TokenType::Block))
                        parse_block(*class_block, new_context);
                }
            }
        }
        break;
        case TokenType::Include:
        {
            line_after_last_include = reader->line + 1;
            std::string& include    = *reader.consume<Include>(TokenType::Include);
            if (std::filesystem::path(include).lexically_normal() == generated_header_include_path)
                b_found_include = true;
            break;
        }
        case TokenType::ReflectedClassBody:
        {
            auto& body = *reader.consume<ReflectedClassBody>(TokenType::ReflectedClassBody);
            if (context.class_stack.empty())
                error("REFLECT_BODY() should not be declared outside classes", body.line, body.column);

            std::string absolute_class_name;
            for (const auto& elem : context.class_stack)
                absolute_class_name += "::" + elem.name;
            if (reflected_classes.contains(absolute_class_name))
                error("Cannot implement multiple REFLECT_BODY() for the same class", body.line, body.column);
            reflected_classes.emplace(absolute_class_name, ReflectedClass{context, body.line});
        }
        break;
        case TokenType::Block:
            if (auto* class_block = reader.consume<Block>(TokenType::Block))
                parse_block(*class_block, context);
            break;
        case TokenType::Arguments:
            reader.consume<Arguments>(TokenType::Arguments);
            break;
        case TokenType::Number:
            reader.consume<Number>(TokenType::Number);
            break;
        default:
            std::cerr << "unhandled token type" << std::endl;
            exit(-2);
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

void HeaderParser::error(const std::string& message, size_t line, size_t column) const
{
    std::cerr << "Error in " << header_path.lexically_normal().string() << ":" << line << ":" << column << " : " << message << "\n";
    exit(-1);
}