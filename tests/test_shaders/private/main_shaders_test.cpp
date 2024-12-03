#include "shader_compiler/shader_compiler.hpp"

#include <filesystem>
#include <iostream>

int main()
{
    ShaderCompiler::CompilationResult result;

    auto session = ShaderCompiler::Compiler::get().create_session("default_mesh");

    auto res = session->compile("gbuffers", session->get_default_permutations_description());

    if (!res.errors.empty())
    {
        for (const auto& error : res.errors)
            std::cerr << error.message << std::endl;

        while (true)
        {
        }
    }
}