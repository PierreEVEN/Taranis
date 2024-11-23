#include "shader_compiler/shader_compiler.hpp"

#include "slang.h"
#include "slang-com-ptr.h"

#include <iostream>
#include <mutex>

namespace ShaderCompiler
{
static Compiler* compiler_instance = nullptr;

Compiler& Compiler::get()
{
    if (!compiler_instance)
        compiler_instance = new Compiler();
    return *compiler_instance;
}

Compiler::~Compiler()
{
    global_session->release();
}

std::shared_ptr<Session> Compiler::create_session() { return std::shared_ptr<Session>(new Session(this)); }

slang::IModule* Compiler::load_module(const std::filesystem::path& module_path)
{
    if (exists(module_path))
    {

    }
    return nullptr;
}

Compiler::Compiler()
{
    createGlobalSession(&global_session);
}

Session::Session(Compiler* in_compiler) : compiler(in_compiler)
{
    slang::SessionDesc sessionDesc;

    // Target
    slang::TargetDesc targetDesc;
    targetDesc.format       = SLANG_SPIRV;
    sessionDesc.targets     = &targetDesc;
    sessionDesc.targetCount = 1;

    // Search paths
    const char* searchPaths[]   = {"resources/shaders"};
    sessionDesc.searchPaths     = searchPaths;
    sessionDesc.searchPathCount = 1;

    std::vector<slang::CompilerOptionEntry> compiler_options;
    compiler_options.emplace_back(slang::CompilerOptionEntry{slang::CompilerOptionName::UseUpToDateBinaryModule, slang::CompilerOptionValue{.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1}});
    sessionDesc.compilerOptionEntries    = compiler_options.data();
    sessionDesc.compilerOptionEntryCount = compiler_options.size();

    compiler->global_session->createSession(sessionDesc, &session);
}

Session::~Session()
{
    session->release();
}

void Session::compile(const std::filesystem::path& path)
{
    Slang::ComPtr<slang::IBlob> diagnostics;
    slang::IModule*             module = session->loadModule(path.string().c_str(), diagnostics.writeRef());
    if (diagnostics)
    {
        std::cerr << static_cast<const char*>(diagnostics->getBufferPointer()) << std::endl;
        exit(-1);
    }

    std::cout << "loaded module : " << module->getFilePath() << "\n";

    std::cout << "compile " << module->getDefinedEntryPointCount() << " entry points...\n";
    for (SlangInt32 i = 0; i < module->getDefinedEntryPointCount(); ++i)
    {
        Slang::ComPtr<slang::IEntryPoint> entry_point;
        module->getDefinedEntryPoint(i, entry_point.writeRef());

        slang::IComponentType*               components[] = {entry_point};
        Slang::ComPtr<slang::IComponentType> program;
        session->createCompositeComponentType(components, 1, program.writeRef());

        slang::ProgramLayout* shaderReflection = program->getLayout();
        std::cout << "[[parameters]]\n";
        for (unsigned pp = 0; pp < shaderReflection->getParameterCount(); pp++)
        {
            slang::VariableLayoutReflection* parameter = shaderReflection->getParameterByIndex(pp);
            std::cout << "\t-" << parameter->getName() << " : " << parameter->getType()->getName() << "\n";
        }

        std::cout << "[[types]]\n";
        for (unsigned pp = 0; pp < shaderReflection->getTypeParameterCount(); pp++)
        {
            slang::TypeParameterReflection* parameter = shaderReflection->getTypeParameterByIndex(pp);
            std::cout << "\t-" << parameter->getName() << " : " << parameter->getName() << "\n";
        }

        std::cout << "[[entry points]]\n";
        for (SlangUInt ee = 0; ee < shaderReflection->getEntryPointCount(); ee++)
        {
            slang::EntryPointReflection* entryPoint = shaderReflection->getEntryPointByIndex(ee);

            auto layout = entryPoint->getTypeLayout()->getType();

            std::cout << "\t- Type layout :: (KIND=" << (int)layout->getKind() << ")\n";
            
            for (size_t n = 0; n < layout->getFieldCount(); ++n)
            {
                std::cout << layout->getFieldByIndex(n)->getName() << "\n";
            }



            std::cout << entryPoint->getName() << " :\n\tparams:\n";
            for (SlangUInt j = 0; j < entryPoint->getParameterCount(); j++)
            {
                std::cout << "\t- N=";
                if (auto name = entryPoint->getParameterByIndex(j)->getName())
                    std::cout << name;
                std::cout << " V=";

                if (auto name = entryPoint->getParameterByIndex(j)->getType()->getName())
                    std::cout << name;
                std::cout << "\n";
            }
        }

        Slang::ComPtr<slang::IComponentType> linkedProgram;
        Slang::ComPtr<ISlangBlob>            diagnosticBlob;
        program->link(linkedProgram.writeRef(), diagnosticBlob.writeRef());

        Slang::ComPtr<slang::IBlob> kernelBlob;
        linkedProgram->getEntryPointCode(0, 0, kernelBlob.writeRef(), diagnostics.writeRef());
    }
}

std::optional<CompilationError> Compiler::compile_raw(const ShaderParser& parser, const Parameters& params, CompilationResult& compilation_result) const
{
    slang::SessionDesc sessionDesc;

    // Target
    slang::TargetDesc targetDesc;
    targetDesc.format       = SLANG_SPIRV;
    sessionDesc.targets     = &targetDesc;
    sessionDesc.targetCount = 1;

    // Search paths
    const char* searchPaths[]   = {"resources/shaders"};
    sessionDesc.searchPaths     = searchPaths;
    sessionDesc.searchPathCount = 1;

    std::vector<slang::CompilerOptionEntry> compiler_options;
    compiler_options.emplace_back(slang::CompilerOptionEntry{slang::CompilerOptionName::UseUpToDateBinaryModule, slang::CompilerOptionValue{.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1}});
    sessionDesc.compilerOptionEntries    = compiler_options.data();
    sessionDesc.compilerOptionEntryCount = compiler_options.size();

    Slang::ComPtr<slang::ISession> session;
    global_session->createSession(sessionDesc, session.writeRef());

    Slang::ComPtr<slang::IBlob> diagnostics;
    slang::IModule*             module = session->loadModule("default_mesh", diagnostics.writeRef());

    std::cout << "loaded module : " << module->getFilePath() << "\n";

    if (diagnostics)
    {
        std::cerr << static_cast<const char*>(diagnostics->getBufferPointer()) << std::endl;
        exit(-1);
    }

    std::cout << "compile " << module->getDefinedEntryPointCount() << " entry points...\n";
    for (SlangInt32 i = 0; i < module->getDefinedEntryPointCount(); ++i)
    {
        Slang::ComPtr<slang::IEntryPoint> entry_point;
        module->getDefinedEntryPoint(i, entry_point.writeRef());

        slang::IComponentType*               components[] = {entry_point};
        Slang::ComPtr<slang::IComponentType> program;
        session->createCompositeComponentType(components, 1, program.writeRef());

        slang::ProgramLayout* shaderReflection = program->getLayout();
        std::cout << "[[parameters]]\n";
        for (unsigned pp = 0; pp < shaderReflection->getParameterCount(); pp++)
        {
            slang::VariableLayoutReflection* parameter = shaderReflection->getParameterByIndex(pp);
            std::cout << "\t-" << parameter->getName() << " : " << parameter->getType()->getName() << "\n";
        }

        std::cout << "[[entry points]]\n";
        for (SlangUInt ee = 0; ee < shaderReflection->getEntryPointCount(); ee++)
        {
            slang::EntryPointReflection* entryPoint = shaderReflection->getEntryPointByIndex(ee);
            std::cout << entryPoint->getName() << " :\n\tparams:\n";
            for (SlangUInt j = 0; j < entryPoint->getParameterCount(); j++)
            {
                std::cout << "\t- N=";
                if (auto name = entryPoint->getParameterByIndex(j)->getName())
                    std::cout << name;
                std::cout << " V=";

                if (auto name = entryPoint->getParameterByIndex(j)->getType()->getName())
                    std::cout << name;
                std::cout << "\n";
            }
        }

        Slang::ComPtr<slang::IComponentType> linkedProgram;
        Slang::ComPtr<ISlangBlob>            diagnosticBlob;
        program->link(linkedProgram.writeRef(), diagnosticBlob.writeRef());

        Slang::ComPtr<slang::IBlob> kernelBlob;
        linkedProgram->getEntryPointCode(0, 0, kernelBlob.writeRef(), diagnostics.writeRef());

    }

    return {};
}

std::optional<CompilationError> Compiler::pre_compile_shader(const std::filesystem::path& path)
{
    std::filesystem::path precompiled_path = std::filesystem::path(SHADER_INTERMEDIATE_DIR) / path;

    /* CREATE SESSION */
    slang::SessionDesc session_desc;

    // Target
    slang::TargetDesc target_desc;
    target_desc.format       = SLANG_SPIRV;
    session_desc.targets     = &target_desc;
    session_desc.targetCount = 1;

    // Search paths
    std::vector<const char*> searchPaths = {"resources/shaders", "resources"};
    session_desc.searchPaths             = searchPaths.data();
    session_desc.searchPathCount         = searchPaths.size();

    Slang::ComPtr<slang::ISession> session;
    global_session->createSession(session_desc, session.writeRef());

    Slang::ComPtr<ISlangBlob> binaryBlob;

    /*
    Slang::ComPtr<ISlangMutableFileSystem> memoryFileSystem = Slang::ComPtr<ISlangMutableFileSystem>(new Slang::MemoryFileSystem());
    memoryFileSystem->loadFile("cache/precompiled-module-imported.slang-module", binaryBlob.writeRef());

    session->isBinaryModuleUpToDate(path.string().c_str(), binaryBlob);
    */

    /* CREATE MAIN MODULE */
    Slang::ComPtr<slang::IBlob> diagnostics;

    slang::IModule* module = session->loadModule("default_mesh", diagnostics.writeRef());

    module->writeToFile("compile.o");
    return {};
}

} // namespace Eng::Gfx