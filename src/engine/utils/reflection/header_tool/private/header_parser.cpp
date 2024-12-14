#include "header_parser.hpp"

#include "llp/file_data.hpp"
#include "llp/lexer.hpp"
#include "llp/parser.hpp"

#include <filesystem>
#include <iostream>

HeaderParser::HeaderParser(const std::string& header_data, std::filesystem::path in_generated_header_include_path, std::filesystem::path in_header_path)
    : generated_header_include_path(std::move(in_generated_header_include_path)), header_path(std::move(in_header_path))
{
    Llp::Lexer lexer(header_data);
    if (auto error = parse_block(lexer.get_root(), {}))
    {
        std::cerr << "Failed to generate reflection data for " << header_path.string() << ":" << error->location.line << ":" << error->location.column << " : " << error->message << "\n";
        exit(-1);
    }
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

std::optional<Llp::ParserError> HeaderParser::parse_block(const Llp::Block& block, const ParserContext& context)
{
    for (Llp::Parser parser(block, {Llp::ELexerToken::Whitespace, Llp::ELexerToken::Comment, Llp::ELexerToken::Endl}); parser;)
    {
        switch (parser.get_current_token_type())
        {
        case Llp::ELexerToken::Symbol:
            parser.consume<Llp::SymbolToken>();
            break;
        case Llp::ELexerToken::Word:
        {
            const std::string& word = parser.consume<Llp::WordToken>()->word;

            // Class declaration
            if (word == "class")
            {
                if (auto class_name = parser.consume<Llp::WordToken>())
                {
                    // Skip final keyword
                    parser.consume<Llp::WordToken>("final");

                    std::vector<std::string> parents;
                    if (parser.consume<Llp::SymbolToken>(':'))
                    {
                        // skip Namespace::Class fields
                        if (!parser.consume<Llp::SymbolToken>(':'))
                            do
                            {
                                // Skip fields
                                parser.consume<Llp::WordToken>("public") || parser.consume<Llp::WordToken>("protected") || parser.consume<Llp::WordToken>("private");

                                if (auto parent_class = parser.consume<Llp::WordToken>())
                                {
                                    std::string parent = parent_class->word;

                                    if (parser.consume<Llp::SymbolToken>('<'))
                                    {
                                        size_t template_level = 1;
                                        do
                                        {
                                            if (parser.consume<Llp::SymbolToken>('<'))
                                            {
                                                parent += '<';
                                                template_level++;
                                            }
                                            else if (auto* template_str = parser.consume<Llp::WordToken>())
                                                parent += template_str->word;
                                            else if (parser.consume<Llp::SymbolToken>('>'))
                                            {
                                                parent += '>';
                                                template_level--;
                                            }

                                        } while (parser && template_level != 0);
                                    }

                                    parents.push_back(parent);
                                }
                                else
                                    return Llp::ParserError{parser.current_location(), "Expected class name"};
                            } while (parser.consume<Llp::ComaToken>());
                    }

                    if (auto* class_block = parser.consume<Llp::BlockToken>())
                        if (auto error = parse_block(class_block->content, context.push_class(ClassDefinition{class_name->word, parents})))
                            return error;
                }
            }
            else if (word == "namespace")
            {
                std::vector<std::string> added_namespace_stack;
                bool                     b_failed = false;
                do
                {
                    parser.consume<Llp::SymbolToken>(':');
                    parser.consume<Llp::SymbolToken>(':');
                    if (auto namespace_name = parser.consume<Llp::WordToken>())
                        added_namespace_stack.push_back(namespace_name->word);
                    else
                    {
                        b_failed = true;
                        break;
                    }

                } while (parser.get_current_token_type() != Llp::ELexerToken::Block);
                if (!b_failed)
                {
                    ParserContext new_context = context;
                    for (const auto& elem : added_namespace_stack)
                        new_context = new_context.push_namespace(elem);
                    if (auto* class_block = parser.consume<Llp::BlockToken>())
                        if (auto error = parse_block(class_block->content, new_context))
                            return error;
                }
            }
            if (word == "REFLECT_BODY")
            {
                if (context.class_stack.empty())
                    return Llp::ParserError{parser.current_location(), "REFLECT_BODY() should not be declared outside classes"};

                std::string absolute_class_name;
                for (const auto& elem : context.class_stack)
                    absolute_class_name += "::" + elem.name;
                if (reflected_classes.contains(absolute_class_name))
                    return Llp::ParserError{parser.current_location(), "Cannot implement multiple REFLECT_BODY() for the same class"};
                reflected_classes.emplace(absolute_class_name, ReflectedClass{context, parser.current_location().line + 1});
            }
        }
        break;
        case Llp::ELexerToken::Include:
        {
            line_after_last_include = parser.current_location().line + 2;
            std::string& include    = parser.consume<Llp::IncludeToken>()->path;
            if (std::filesystem::path(include).lexically_normal() == generated_header_include_path)
                b_found_include = true;
        }
        break;
        case Llp::ELexerToken::Block:
            if (auto* class_block = parser.consume<Llp::BlockToken>())
                if (auto error = parse_block(class_block->content, context))
                    return error;
            break;
        default:
            ++parser;
        }
    }
    return {};
}

bool HeaderParser::parse_check_include(TextReader& reader, const std::filesystem::path& desired_path)
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