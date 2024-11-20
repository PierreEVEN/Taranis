#include "shader_compiler/shader_parser.hpp"

#include "llp/parser.hpp"

namespace ShaderCompiler
{

ShaderParser::ShaderParser(const std::string& source_shader) : lexer(source_shader), source_code(source_shader)
{
    error = lexer.get_error();


    if (error)
        return;
    error = parse();
}

std::optional<Llp::ParserError> ShaderParser::parse()
{
    for (Llp::Parser parser(lexer.get_root(), {Llp::ELexerToken::Whitespace, Llp::ELexerToken::Comment}); parser; ++parser)
    {
        if (parser.consume<Llp::WordToken>("option"))
        {
            if (Llp::WordToken* option_name = parser.consume<Llp::WordToken>())
            {
                if (parser.consume<Llp::EqualsToken>())
                {
                    if (parser.consume<Llp::WordToken>("true"))
                        options.insert_or_assign(option_name->word, true);
                    else if (parser.consume<Llp::WordToken>("false"))
                        options.insert_or_assign(option_name->word, true);
                    else
                        return Llp::ParserError{parser.current_location(), "expected true or false"};

                    if (!parser.consume<Llp::SemicolonToken>())
                        return Llp::ParserError{parser.current_location(), "; expected"};
                }
                else if (parser.consume<Llp::SemicolonToken>())
                    options.emplace(option_name->word, false);
                else
                    return Llp::ParserError{parser.current_location(), "; expected"};
            }
            else
                return Llp::ParserError{parser.current_location(), "expected option name"};
        }
        else if (parser.consume<Llp::WordToken>("pass"))
        {
            if (Llp::ArgumentsToken* option_name = parser.consume<Llp::ArgumentsToken>())
            {
                std::vector<std::string> pass_list;
                if (auto args_error = parse_pass_args(*option_name, pass_list))
                    return args_error;

                if (auto block = parser.consume<Llp::BlockToken>())
                {
                    auto block_shared = std::make_shared<ShaderBlock>();
                    block_shared->start = block->location;
                    block_shared->end   = block->end;
                    block_shared->raw_code = source_code.substr(block->location.index, block->end.index - block->location.index);
                    for (const auto& pass : pass_list)
                        passes.insert_or_assign(pass, std::vector<ShaderBlock>{}).first->second.emplace_back(block_shared);
                }
                else
                    return Llp::ParserError{parser.current_location(), "expected {shader_block}"};
            }
            else
                return Llp::ParserError{parser.current_location(), "expected (args)"};
        }
    }
    return {};
}

std::optional<Llp::ParserError> ShaderParser::parse_pass_args(Llp::ArgumentsToken& args, std::vector<std::string>& pass_list)
{
}
}