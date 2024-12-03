#include "shader_compiler/shader_parser.hpp"

#include "llp/parser.hpp"

#include <format>
#include <iostream>

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
    for (Llp::Parser parser(lexer.get_root(), {Llp::ELexerToken::Whitespace, Llp::ELexerToken::Comment, Llp::ELexerToken::Endl}); parser; ++parser)
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
                    auto block_shared      = std::make_shared<ShaderBlock>();
                    block_shared->start    = block->location;
                    block_shared->end      = block->end;
                    block_shared->raw_code = source_code.substr(block->location.index, block->end.index - block->location.index);
                    if (auto block_error = parse_block(*block, *block_shared))
                        return block_error;
                    for (const auto& pass : pass_list)
                        passes.insert_or_assign(pass, std::vector<std::shared_ptr<ShaderBlock>>{}).first->second.emplace_back(block_shared);
                }
                else
                    return Llp::ParserError{parser.current_location(), std::string("expected {shader_block}, found ") + Llp::token_type_to_string(parser.get_current_token_type())};
            }
            else
                return Llp::ParserError{parser.current_location(), "expected (args)"};
        }
        else if (auto* config_name = parser.consume<Llp::WordToken>())
        {
            if (parser.consume<Llp::EqualsToken>())
            {
                if (auto* config_value = parser.consume<Llp::WordToken>())
                {
                    if (parser.consume<Llp::SemicolonToken>())
                    {
                        if (const auto config_error = parse_config_value(config_name->word, config_value->word))
                            return Llp::ParserError{parser.current_location(), *config_error};
                    }
                    else
                        return Llp::ParserError{parser.current_location(), "';' expected"};
                }
                else
                    return Llp::ParserError{parser.current_location(), "value expected"};
            }
            else
                return Llp::ParserError{parser.current_location(), "'=' expected"};
        }
        else
            return Llp::ParserError{parser.current_location(), std::format("Unexpected token : Found {}", Llp::token_type_to_string(parser.get_current_token_type()))};
    }
    return {};
}

std::optional<Llp::ParserError> ShaderParser::parse_pass_args(Llp::ArgumentsToken& args, std::vector<std::string>& pass_list)
{
    for (Llp::Parser parser(args.content, {Llp::ELexerToken::Whitespace, Llp::ELexerToken::Comment, Llp::ELexerToken::Endl}); parser; ++parser)
    {
        if (auto* name = parser.consume<Llp::WordToken>())
            pass_list.emplace_back(name->word);
        else if (!parser.consume<Llp::ComaToken>())
            return Llp::ParserError{parser.current_location(), "unexpected token"};
    }
    return {};
}

std::optional<std::string> ShaderParser::parse_config_value(const std::string& key, const std::string& value)
{
    if (key == "culling")
    {
        if (value == "none")
            pipeline_options.culling = Eng::Gfx::ECulling::None;
        else if (value == "front")
            pipeline_options.culling = Eng::Gfx::ECulling::Front;
        else if (value == "back")
            pipeline_options.culling = Eng::Gfx::ECulling::Back;
        else if (value == "both")
            pipeline_options.culling = Eng::Gfx::ECulling::Both;
        else
            return {"Invalid value. Expected none, front, back or both. Found " + value};
    }
    else if (key == "front_face")
    {
        if (value == "clockwise")
            pipeline_options.front_face = Eng::Gfx::EFrontFace::Clockwise;
        else if (value == "counter_clockwise")
            pipeline_options.front_face = Eng::Gfx::EFrontFace::Clockwise;
        else
            return {"Invalid value. Expected clockwise or counter_clockwise. Found " + value};
    }
    else if (key == "topology")
    {
        if (value == "lines")
            pipeline_options.topology = Eng::Gfx::ETopology::Lines;
        else if (value == "points")
            pipeline_options.topology = Eng::Gfx::ETopology::Points;
        else if (value == "triangles")
            pipeline_options.topology = Eng::Gfx::ETopology::Triangles;
        else
            return {"Invalid value. Expected lines, points or triangles. Found " + value};
    }
    else if (key == "polygon")
    {
        if (value == "fill")
            pipeline_options.polygon = Eng::Gfx::EPolygonMode::Fill;
        else if (value == "line")
            pipeline_options.polygon = Eng::Gfx::EPolygonMode::Line;
        else if (value == "point")
            pipeline_options.polygon = Eng::Gfx::EPolygonMode::Point;
        else
            return {"Invalid value. Expected fill, line or point. Found " + value};
    }
    else if (key == "alpha")
    {
        if (value == "additive")
            pipeline_options.alpha = Eng::Gfx::EAlphaMode::Additive;
        else if (value == "opaque")
            pipeline_options.alpha = Eng::Gfx::EAlphaMode::Opaque;
        else if (value == "translucent")
            pipeline_options.alpha = Eng::Gfx::EAlphaMode::Translucent;
        else
            return {"Invalid value. Expected additive, opaque or translucent. Found " + value};
    }
    else
        return {"Invalid config. Expected culling, front_face, topology, polygon or alpha. Found " + key};
    return {};
}

std::optional<Llp::ParserError> ShaderParser::parse_block(const Llp::BlockToken& args, ShaderBlock& block)
{
    for (Llp::Parser parser(args.content, {Llp::ELexerToken::Whitespace, Llp::ELexerToken::Comment, Llp::ELexerToken::Endl}); parser; ++parser)
    {
        if (parser.get<Llp::WordToken>(0) && parser.get<Llp::WordToken>(1))
        {
            // Find functions declarations
            if (parser.get<Llp::ArgumentsToken>(2))
            {
                parser.consume<Llp::WordToken>();
                EntryPoint      ep;
                Llp::WordToken* func_name = parser.consume<Llp::WordToken>();
                ep.name                   = func_name->word;

                if (func_name->word == "fragment")
                {
                    ep.stage = Eng::Gfx::EShaderStage::Fragment;
                    block.entry_point.emplace_back(ep);
                }
                else if (func_name->word == "vertex")
                {
                    ep.stage = Eng::Gfx::EShaderStage::Vertex;
                    block.entry_point.emplace_back(ep);
                }
                else if (func_name->word == "compute")
                {
                    ep.stage = Eng::Gfx::EShaderStage::Compute;
                    block.entry_point.emplace_back(ep);
                }
                parser.consume<Llp::ArgumentsToken>();
            }
        }
    }
    return {};
}
} // namespace ShaderCompiler