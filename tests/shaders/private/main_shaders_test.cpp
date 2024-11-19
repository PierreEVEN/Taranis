#include "shader_compiler/shader_compiler.hpp"

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

}