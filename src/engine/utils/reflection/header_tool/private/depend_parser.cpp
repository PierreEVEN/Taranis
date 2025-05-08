#include "depend_parser.h"

#include <iostream>

#include "llp/file_data.hpp"
#include "llp/lexer.hpp"
#include "llp/native_tokens.hpp"
#include "llp/parser.hpp"

DependParser::DependParser(const std::filesystem::path& path)
{
    if (auto error = parse(path))
    {
        std::cout << path << ":" << error->location.line << ":" << error->location.column << " : " << "Failed to parse depend file : " << error->message << "\n";
        exit(EXIT_FAILURE);
    }
}

std::time_t DependParser::get_last_write_time() const
{
    std::time_t max = 0;
    for (const auto& dep : dependencies)
    {
        auto ftime = last_write_time(dep);
        auto sctp  = std::chrono::system_clock::to_time_t(std::chrono::clock_cast<std::chrono::system_clock>(ftime));
        if (sctp > max)
            max = sctp;
    }
    return max;
}


/*####[ [[string]] ]####*/
DECLARE_LEXER_TOKEN(MsvcDependLiteralToken)

    static std::unique_ptr<MsvcDependLiteralToken> consume(Llp::Lexer&, Llp::Location& in_location, const std::string& source, std::optional<Llp::ParserError>& error)
    {
        if (source[in_location.index] == '[' && source[in_location.index + 1] == '[')
        {
            ++++in_location;
            Llp::Location start = in_location;
            for (size_t i = start.index; i < source.length() - 1; ++i)
            {
                if (source[i] == ']' && source[i + 1] == ']')
                {
                    std::string value = source.substr(start.index, in_location.index - start.index);
                    ++++in_location;
                    auto comment   = std::make_unique<MsvcDependLiteralToken>(start);
                    comment->value = value;
                    return comment;
                }
                if (source[i] == '\n')
                    in_location.on_new_line();
                else
                    ++in_location;
            }
            error = {start, "String comment doesn't end. '*/' expected"};
        }
        return nullptr;
    }

    std::string to_string(bool) const override
    {
        return "[[" + value + "]]";
    }

    std::string value;
};

std::optional<Llp::ParserError> DependParser::parse(const std::filesystem::path& path)
{
    if (!exists(path))
        return {};
    auto source_header = std::make_shared<FileReader>(path);
    source_header->read();
    Llp::Lexer lexer;
    lexer.register_token_before<MsvcDependLiteralToken, Llp::StringLiteralToken>("MsvcLiteralString");

    lexer.run(source_header->raw_stream());
    if (auto lexer_error = lexer.get_error())
    {
        std::cerr << path << ":" << lexer_error->location.line << ":" << lexer_error->location.column << " : " << lexer_error->message << "\n";
        exit(EXIT_FAILURE);
    }
    Llp::Parser main_parser(lexer.get_root());

    if (auto main_block = main_parser.consume<Llp::BlockToken>())
    {
        Llp::Parser key_list_block(main_block->content);
        do
        {
            auto key = key_list_block.consume<Llp::WordToken>();
            if (!key_list_block.consume<Llp::EqualsToken>())
                return Llp::ParserError{key_list_block.current_location(), "expected '='"};
            if (key->word == "files")
            {
                if (auto dep_list_block = key_list_block.consume<Llp::BlockToken>())
                {
                    Llp::Parser dependencies_block_parser(dep_list_block->content);
                    do
                    {
                        if (auto dep_path = dependencies_block_parser.consume<Llp::StringLiteralToken>())
                            dependencies.emplace_back(dep_path->value);
                        else if (auto dep_path = dependencies_block_parser.consume<MsvcDependLiteralToken>())
                            dependencies.emplace_back(dep_path->value);
                        else
                            return Llp::ParserError{key_list_block.current_location(), "expected string literal : \"\""};
                        if (dependencies_block_parser.get_current_token_type() == Llp::NULL_TOKEN)
                        {
                            ++dependencies_block_parser;
                            break;
                        }
                        if (!dependencies_block_parser.consume<Llp::ComaToken>())
                            return Llp::ParserError{key_list_block.current_location(), "expected coma : ,"};
                    } while (dependencies_block_parser.consume<Llp::ComaToken>());
                    if (dependencies_block_parser.get_current_token_type() != Llp::NULL_TOKEN)
                        return Llp::ParserError{key_list_block.current_location(), std::format("expected end of block, got {}", lexer.get_context().get_token_name(key_list_block.get_current_token_type()))};
                    return {};
                }
                return Llp::ParserError{key_list_block.current_location(), std::format("expected file block, got {}", lexer.get_context().get_token_name(key_list_block.get_current_token_type()))};
            }
            if (!key_list_block.consume<Llp::BlockToken>())
                return Llp::ParserError{key_list_block.current_location(), "expected {block}"};
        } while (key_list_block.consume<Llp::ComaToken>());

        if (key_list_block.get_current_token_type() != Llp::NULL_TOKEN)
            return Llp::ParserError{key_list_block.current_location(), std::format("expected end of block, got {}", lexer.get_context().get_token_name(key_list_block.get_current_token_type()))};
    }
    else
    {
        return Llp::ParserError{main_parser.current_location(), std::format("expected main block, got {}", lexer.get_context().get_token_name(main_parser.get_current_token_type()))};
    }
    return {};
}