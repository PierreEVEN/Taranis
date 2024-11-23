#include "shader_compiler/shader_compiler.hpp"

#include "slang.h"
#include "slang-com-ptr.h"

#include <iostream>
#include <mutex>

#include <spirv_reflect.h>

#define SLANG_RETURN_ON_FAIL_2(x)           \
    {                                       \
        SlangResult _res = (x);             \
        if (SLANG_FAILED(_res))             \
        {                                   \
            SLANG_HANDLE_RESULT_FAIL(_res); \
            std::cerr << "FAILEDD !!!\n";   \
            return;                         \
        }                                   \
    }

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
    SLANG_RETURN_ON_FAIL_2(createGlobalSession(&global_session));
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
    compiler_options.emplace_back(slang::CompilerOptionEntry{slang::CompilerOptionName::Optimization, slang::CompilerOptionValue{.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 3}});
    sessionDesc.compilerOptionEntries    = compiler_options.data();
    sessionDesc.compilerOptionEntryCount = compiler_options.size();

    SLANG_RETURN_ON_FAIL_2(compiler->global_session->createSession(sessionDesc, &session));
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
        exit(1);
    }

    std::cout << "loaded module : " << module->getFilePath() << "\n";

    slang::ProgramLayout* layout = module->getLayout(0, diagnostics.writeRef());
    if (diagnostics)
    {
        std::cerr << static_cast<const char*>(diagnostics->getBufferPointer()) << std::endl;
        exit(2);
    }

    for (size_t p = 0; p < layout->getParameterCount(); ++p)
    {
        auto parameter = layout->getParameterByIndex(p);
        std::cout << "\t- PARAMETER " << parameter->getName() << " = " << parameter->getType()->getName() << " : " << (int)parameter->getStage() << "\n";
        for (size_t at = 0; at < parameter->getVariable()->getUserAttributeCount(); ++at)
        {
            auto user_attribute = parameter->getVariable()->getUserAttributeByIndex(at);
            for (size_t ar = 0; ar < user_attribute->getArgumentCount(); ++ar)
            {
                std::cout << "\t\t- " << user_attribute->getArgumentType(ar)->getName() << "\n";
            }
        }
    }

    std::cout << "compile " << module->getDefinedEntryPointCount() << " entry points...\n";
    for (SlangInt32 i = 0; i < module->getDefinedEntryPointCount(); ++i)
    {
        Slang::ComPtr<slang::IEntryPoint> entry_point;
        SLANG_RETURN_ON_FAIL_2(module->getDefinedEntryPoint(i, entry_point.writeRef()));

        slang::IComponentType*               components[] = {entry_point};
        Slang::ComPtr<slang::IComponentType> program;
        SLANG_RETURN_ON_FAIL_2(session->createCompositeComponentType(components, 1, program.writeRef()));

        slang::ProgramLayout* shaderReflection = program->getLayout();

        std::cout << "[[entry points]]\n";
        for (SlangUInt ee = 0; ee < shaderReflection->getEntryPointCount(); ee++)
        {
            slang::EntryPointReflection* entryPoint = shaderReflection->getEntryPointByIndex(ee);

            std::cout << "\t<<user attributes>>\n";
            auto global_var = entryPoint->getFunction();
            for (size_t at = 0; at < global_var->getUserAttributeCount(); ++at)
            {
                auto user_attribute = global_var->getUserAttributeByIndex(at);
                for (size_t ar = 0; ar < user_attribute->getArgumentCount(); ++ar)
                {
                    size_t      size;
                    const char* str = user_attribute->getArgumentValueString(ar, &size);
                    std::string string(str, size);
                    std::cout << "\t\t- " << string << "\n";
                }
            }

            std::cout << "\tMAIN = '" << entryPoint->getName() << "':\n";
            for (SlangUInt j = 0; j < entryPoint->getParameterCount(); j++)
            {
                auto param = entryPoint->getParameterByIndex(j);
                std::cout << "\t\t- " << param->getType()->getName() << " " << param->getName() << "\n";
            }
        }

        Slang::ComPtr<slang::IComponentType> linkedProgram;
        Slang::ComPtr<ISlangBlob>            diagnosticBlob;
        program->link(linkedProgram.writeRef(), diagnosticBlob.writeRef());
        if (diagnostics)
        {
            std::cerr << static_cast<const char*>(diagnostics->getBufferPointer()) << std::endl;
            exit(3);
        }

        shaderReflection = linkedProgram->getLayout();

        std::cout << "[[parameters]]\n";
        for (unsigned pp = 0; pp < shaderReflection->getParameterCount(); pp++)
        {
            slang::VariableLayoutReflection* parameter = shaderReflection->getParameterByIndex(pp);
            std::cout << "\t-" << parameter->getName() << " : " << parameter->getType()->getName() << "\n";

            std::cout << "TEST : category = " << (int)parameter->getCategory() << "\n";
            std::cout << "TEST : stage    = " << (int)parameter->getStage() << "\n";
            std::cout << "TEST : space    = " << (int)parameter->getBindingSpace() << "\n";
        }

        std::cout << "[[types]]\n";
        for (unsigned pp = 0; pp < shaderReflection->getTypeParameterCount(); pp++)
        {
            slang::TypeParameterReflection* parameter = shaderReflection->getTypeParameterByIndex(pp);
            std::cout << "\t-" << parameter->getName() << " : " << parameter->getName() << "\n";
        }


        Slang::ComPtr<slang::IBlob> kernelBlob;
        linkedProgram->getEntryPointCode(0, 0, kernelBlob.writeRef(), diagnostics.writeRef());

        std::vector<uint8_t> buffer(kernelBlob->getBufferSize());
        memcpy(buffer.data(), kernelBlob->getBufferPointer(), kernelBlob->getBufferSize());
        extract_spirv_properties(buffer, "main");

        std::cout << "\n\n";

    }
}

std::optional<CompilationError> Compiler::compile_raw(const ShaderParser& parser, const Parameters& params, CompilationResult& compilation_result) const
{

    return {};
}

std::optional<CompilationError> Compiler::pre_compile_shader(const std::filesystem::path& path)
{

    return {};
}

void Session::extract_spirv_properties(const std::vector<uint8_t>& spirv, const std::string& entry_point)
{
    // Generate reflection data for a shader
    SpvReflectShaderModule module;
    SpvReflectResult       result = spvReflectCreateShaderModule(spirv.size(), spirv.data(), &module);

    if (result != SPV_REFLECT_RESULT_SUCCESS)
        exit(10);

    // Enumerate and extract shader's input variables
    uint32_t var_count = 0;
    result             = spvReflectEnumerateEntryPointInputVariables(&module, entry_point.c_str(), &var_count, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        exit(11);
    std::vector<SpvReflectInterfaceVariable*> input_vars(var_count, nullptr);
    result = spvReflectEnumerateEntryPointInputVariables(&module, entry_point.c_str(), &var_count, input_vars.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        exit(12);

    if (!input_vars.empty())
    {
        uint32_t first_offset = input_vars[0]->word_offset.location;

        for (const auto& var : input_vars)
        {
            if (static_cast<int>(var->built_in) >= 0)
                continue;
            //std::cout << "SPV_REFL : Stage inputs loc=" << var->location << " / offset=" << (var->word_offset.location - first_offset) * 2 << " / format=" << var->format << "\n";
        }
    }

    // Enumerate and extract shader's bindings
    result = spvReflectEnumerateEntryPointDescriptorBindings(&module, entry_point.c_str(), &var_count, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        exit(13);
    std::vector<SpvReflectDescriptorBinding*> descriptor_bindings(var_count, nullptr);
    result = spvReflectEnumerateEntryPointDescriptorBindings(&module, entry_point.c_str(), &var_count, descriptor_bindings.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        exit(14);

    for (const auto& binding : descriptor_bindings)
    {
        Eng::Gfx::EBindingType binding_type;
        switch (binding->descriptor_type)
        {
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
            binding_type = Eng::Gfx::EBindingType::SAMPLER;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            binding_type = Eng::Gfx::EBindingType::COMBINED_IMAGE_SAMPLER;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            binding_type = Eng::Gfx::EBindingType::SAMPLED_IMAGE;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            binding_type = Eng::Gfx::EBindingType::STORAGE_IMAGE;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            binding_type = Eng::Gfx::EBindingType::UNIFORM_TEXEL_BUFFER;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            binding_type = Eng::Gfx::EBindingType::STORAGE_TEXEL_BUFFER;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            binding_type = Eng::Gfx::EBindingType::UNIFORM_BUFFER;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            binding_type = Eng::Gfx::EBindingType::STORAGE_BUFFER;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            binding_type = Eng::Gfx::EBindingType::UNIFORM_BUFFER_DYNAMIC;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            binding_type = Eng::Gfx::EBindingType::STORAGE_BUFFER_DYNAMIC;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            binding_type = Eng::Gfx::EBindingType::INPUT_ATTACHMENT;
            break;
        default:
            exit(15);
        }
        std::cout << "SPV_REFL : Bindings " << binding->name << " location=" << binding->binding << " / type=" << (int)binding_type << "\n";
    }

    // Enumerate and extract shader's output
    result = spvReflectEnumerateEntryPointOutputVariables(&module, entry_point.c_str(), &var_count, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        exit(16);
    std::vector<SpvReflectInterfaceVariable*> output_variables(var_count, nullptr);
    result = spvReflectEnumerateEntryPointOutputVariables(&module, entry_point.c_str(), &var_count, output_variables.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        exit(17);

    if (!output_variables.empty())
    {
        uint32_t first_offset = output_variables[0]->word_offset.location;
        for (const auto& var : output_variables)
            ; // std::cout << "SPV_REFL : Stage outputs loc=" << var->location << " / offset=" << (var->word_offset.location - first_offset) * 2 << " / format=" << var->format << "\n";
    }

    // Enumerate and extract push constants
    result = spvReflectEnumerateEntryPointPushConstantBlocks(&module, entry_point.c_str(), &var_count, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        exit(18);
    std::vector<SpvReflectBlockVariable*> push_constants(var_count, nullptr);
    result = spvReflectEnumerateEntryPointPushConstantBlocks(&module, entry_point.c_str(), &var_count, push_constants.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        exit(19);

    if (!push_constants.empty())
    {
        if (push_constants.size() != 1)
            exit(20);
        std::cout << "SPV_REFL : Push constant size =" << push_constants[0]->size << "\n";
    }

    // Output variables, descriptor bindings, descriptor sets, and push constants
    // can be enumerated and extracted using a similar mechanism.

    // Destroy the reflection data when no longer required.
    spvReflectDestroyShaderModule(&module);
}


} // namespace Eng::Gfx