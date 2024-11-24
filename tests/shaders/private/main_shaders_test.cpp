#include "shader_compiler/shader_compiler.hpp"

#include <filesystem>

int main()
{
    ShaderCompiler::CompilationResult result;

    auto session = ShaderCompiler::Compiler::get().create_session("default_mesh");

    session->compile("gbuffers", session->get_default_permutations_description());
}