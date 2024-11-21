#include "shader_compiler/shader_compiler.hpp"
#include "shader_compiler/shader_parser.hpp"

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

    ShaderCompiler::ShaderParser parser(shader_data);

    if (parser.get_error())
    {
        std::cerr << "Parsing failed " << parser.get_error()->location.line << ":" << parser.get_error()->location.column << " : " << parser.get_error()->message << "\n";
        return -1;
    }

    ShaderCompiler::Compiler compiler;

    CompilationResult result;
    if (auto error = compiler.compile_raw(parser, {}, result))
    {
        std::cerr << "Compilation failed " << error->line << ":" << error->column << " : " << error->message << std::endl;
        return -1;
    }

    std::cout << "compiled " << result.compiled_passes.size() << " passes\n";

    return 0;
}