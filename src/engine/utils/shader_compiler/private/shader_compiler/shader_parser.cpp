#include "shader_compiler/shader_parser.hpp"

#include "llp/parser.hpp"

#include <format>

namespace ShaderCompiler
{

ShaderParser::ShaderParser(const std::string& source_shader) : source_code(source_shader)
{
    Llp::TokenSet token_set = Llp::TokenSet::preset_c_like();
    error                   = lexer.tokenize(source_shader, token_set);
    if (error)
        return;
    error = parse(token_set);
}

Llp::ParserError ShaderParser::parse(const Llp::TokenSet& token_set)
{
    for (Llp::Parser parser(lexer); parser; ++parser)
    {
        if (parser.consume<Llp::WordToken>("option"))
        {
            if (Llp::WordToken* option_name = parser.consume<Llp::WordToken>())
            {
                if (parser.consume<Llp::SymbolToken>('='))
                {
                    if (parser.consume<Llp::WordToken>("true"))
                        options.insert_or_assign(option_name->word, true);
                    else if (parser.consume<Llp::WordToken>("false"))
                        options.insert_or_assign(option_name->word, true);
                    else
                        return Llp::ParserError{parser.current_location(), "expected true or false"};

                    if (!parser.consume<Llp::SymbolToken>(';'))
                        return Llp::ParserError{parser.current_location(), "; expected"};
                }
                else if (parser.consume<Llp::SymbolToken>(';'))
                    options.emplace(option_name->word, false);
                else
                    return Llp::ParserError{parser.current_location(), "; expected"};
            }
            else
                return Llp::ParserError{parser.current_location(), "expected option name"};
        }
        else if (parser.consume<Llp::WordToken>("pass"))
        {
            if (Llp::ParenthesisBlockToken* option_name = parser.consume<Llp::ParenthesisBlockToken>())
            {
                std::vector<std::string> pass_list;
                if (auto args_error = parse_pass_args(*option_name, pass_list))
                    return args_error;

                if (auto block = parser.consume<Llp::BraceBlockToken>())
                {
                    auto block_shared      = std::make_shared<ShaderBlock>();
                    block_shared->start    = block->location;
                    block_shared->end      = block->end;
                    block_shared->raw_code = source_code.substr(block->location.get_index(), block->end.get_index() - block->location.get_index());
                    if (auto block_error = parse_block(*block, *block_shared))
                        return block_error;
                    for (const auto& pass : pass_list)
                        passes.insert_or_assign(pass, std::vector<std::shared_ptr<ShaderBlock>>{}).first->second.emplace_back(block_shared);
                }
                else
                    return Llp::ParserError{parser.current_location(), std::string("expected {shader_block}, found ") + parser.get_current_token_name(token_set)};
            }
            else
                return Llp::ParserError{parser.current_location(), "expected (args)"};
        }
        else if (auto* config_name = parser.consume<Llp::WordToken>())
        {
            if (parser.consume<Llp::SymbolToken>('='))
            {
                if (auto* config_value = parser.consume<Llp::WordToken>())
                {
                    if (parser.consume<Llp::SymbolToken>(';'))
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
            return Llp::ParserError{parser.current_location(), std::format("Unexpected token : Found {}", parser.get_current_token_name(token_set))};
    }
    return {};
}

Llp::ParserError ShaderParser::parse_pass_args(Llp::ParenthesisBlockToken& args, std::vector<std::string>& pass_list)
{
    for (Llp::Parser parser(args.content); parser; ++parser)
    {
        if (auto* name = parser.consume<Llp::WordToken>())
            pass_list.emplace_back(name->word);
        else if (!parser.consume<Llp::SymbolToken>(','))
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

Llp::ParserError ShaderParser::parse_block(const Llp::BraceBlockToken& args, ShaderBlock& block)
{
    for (Llp::Parser parser(args.content); parser; ++parser)
    {
        if (parser.get<Llp::WordToken>(0) && parser.get<Llp::WordToken>(1))
        {
            // Find functions declarations
            if (parser.get<Llp::ParenthesisBlockToken>(2))
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
                parser.consume<Llp::ParenthesisBlockToken>();
            }
        }
    }
    return {};
}
} // namespace ShaderCompiler