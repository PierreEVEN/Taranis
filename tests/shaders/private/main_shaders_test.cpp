#include "llp/lexer.hpp"
#include "llp/parser.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int main()
{

    std::string shader_data;

    std::ifstream reader("resources/shaders/default_mesh.hlsl");
    std::string   line;
    while (std::getline(reader, line))
        shader_data += line + "\n";

    Llp::Lexer lexer(shader_data);

    if (lexer.get_error())
    {
        std::cerr << "Parsing failed " << lexer.get_error()->location.line << ":" << lexer.get_error()->location.column << " : " << lexer.get_error()->message << std::endl;
    }

    for (Llp::Parser parser(lexer.get_root(), {Llp::ELexerToken::Whitespace, Llp::ELexerToken::Comment}); parser; ++parser)
    {
        if (parser.consume<Llp::WordToken>("option"))
        {
            if (Llp::WordToken* option_name = parser.consume<Llp::WordToken>())
            {
                if (parser.consume<Llp::EqualsToken>())
                {
                    if (parser.consume<Llp::WordToken>("true"))
                    {
                        std::cout << option_name->word << " false" << std::endl;
                    }
                    else if (parser.consume<Llp::WordToken>("false"))
                    {
                        std::cout << option_name->word << " false" << std::endl;
                    }
                    else
                    {
                        std::cerr << "expected true or false" << std::endl;
                    }

                    if (!parser.consume<Llp::SemicolonToken>())
                        std::cerr << "; expected" << std::endl;
                }
                else if (parser.consume<Llp::SemicolonToken>())
                {
                    std::cout << option_name->word << std::endl;
                }
                else
                {
                    std::cerr << "; expecteds" << std::endl;
                }
            }
            else
                std::cerr << "expected option name" << std::endl;
        }
    }

    //std::cout << lexer.get_root().to_string(true) << std::endl;

    //std::cout << lexer.get_root().to_string(false) << std::endl;

    return 0;
    /*
    Eng::Gfx::ShaderCompiler compiler;

    Eng::Gfx::ShaderSource sources(shader_data);

    for (const auto& err : sources.get_errors())
        std::cout << "parse error | " << err.line << ":" << err.column << " " << err.error_message << "\n";

    if (!sources.get_errors().empty())
        return -1;

    Eng::Gfx::RenderPassSources gbuffer_pass = sources.get_render_pass("GBuffer");

    for (const auto& stage : gbuffer_pass.stage_inputs)
    {
        auto res = compiler.compile_raw(gbuffer_pass.raw, stage.second, stage.first, std::filesystem::path("resources/shaders/default_mesh.hlsl"));

        for (const auto& err : res.errors)
            std::cout << "compilation error | " << err.line << ":" << err.column << " " << err.error_message << "\n";
        if (res.errors.empty())
            std::cout << "compilation succeeded\n";
    }
    */
}