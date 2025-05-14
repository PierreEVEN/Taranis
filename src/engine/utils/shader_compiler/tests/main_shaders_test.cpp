#include "shader_compiler/shader_compiler.hpp"

#include <filesystem>
#include <iostream>

int main()
{
    ShaderCompiler::CompilationResult result;

    auto session = ShaderCompiler::Compiler::get().create_session("default_mesh");

    auto res = session->compile("shadows", session->get_default_permutations_description());

    for (const auto& stage : res.stages)
    {
        std::cout << "Stage : " << static_cast<int>(stage.first) << "\n";
        for (const auto& inp : stage.second.inputs)
            std::cout << "\t - input : " << inp.first << " : loc=" << inp.second.location << ", offset=" << inp.second.offset << ", format=" << static_cast<int>(inp.second.format) << "\n";
    }

    if (!res.errors.empty())
    {
        for (const auto& error : res.errors)
            std::cerr << error.message << std::endl;
    }
}