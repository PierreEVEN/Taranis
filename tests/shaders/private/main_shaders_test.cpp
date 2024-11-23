#include "shader_compiler/shader_compiler.hpp"

#include <filesystem>

int main()
{
    ShaderCompiler::CompilationResult result;

    auto session = ShaderCompiler::Compiler::get().create_session();

    session->compile("default_mesh");
}