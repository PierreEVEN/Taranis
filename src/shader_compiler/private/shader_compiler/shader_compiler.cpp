#include "shader_compiler/shader_compiler.hpp"

#include "slang.h"
#include "slang-com-ptr.h"
#include "slang_com_object.hpp"
#include "slang_helper.hpp"

#include <iostream>
#include <mutex>

#include <spirv_reflect.h>

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

std::shared_ptr<Session> Compiler::create_session(const std::filesystem::path& path)
{
    return std::shared_ptr<Session>(new Session(this, path));
}

slang::IModule* Compiler::load_module(const std::filesystem::path& module_path)
{
    if (exists(module_path))
    {

    }
    return nullptr;
}

Compiler::Compiler()
{
    if (SLANG_FAILED(createGlobalSession(&global_session)))
    {
        std::cerr << "Failed to create global slang compiler session\n";
        exit(-1);
    }
}

Session::Session(Compiler* in_compiler, const std::filesystem::path& path) : compiler(in_compiler)
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
    sessionDesc.compilerOptionEntryCount = static_cast<uint32_t>(compiler_options.size());

    if (SLANG_FAILED(compiler->global_session->createSession(sessionDesc, &session)))
    {
        load_errors.emplace_back("Failed to create slang compiler session");
        return;
    }

    Slang::ComPtr<slang::IBlob> diagnostics;
    module = session->loadModule(path.string().c_str(), diagnostics.writeRef());
    if (diagnostics)
    {
        load_errors.emplace_back(static_cast<const char*>(diagnostics->getBufferPointer()));
        return;
    }

    std::cout << "TEST ::: " << (int)module->getModuleReflection()->getChildrenCount() << "\n";

    slang::ProgramLayout* layout = module->getLayout(0, diagnostics.writeRef());
    if (diagnostics)
    {
        load_errors.emplace_back(static_cast<const char*>(diagnostics->getBufferPointer()));
        return;
    }
}

Session::~Session()
{
    session->release();
}

CompilationResult Session::compile(const std::string& render_pass, const Eng::Gfx::PermutationDescription& permutation) const
{
    CompilationResult result;

    for (const auto& error : load_errors)
        result.push_error(error);
    if (!result.errors.empty())
        return result;

    std::cout << "compile " << module->getDefinedEntryPointCount() << " entry points...\n";
    for (SlangInt32 i = 0; i < module->getDefinedEntryPointCount(); ++i)
    {
        Slang::ComPtr<slang::IEntryPoint> entry_point;
        if (SLANG_FAILED(module->getDefinedEntryPoint(i, entry_point.writeRef())))
            return result.push_error({"Failed to get entry point"});

        std::vector<slang::IComponentType*> components;
        if (!permutation.keys().empty())
        {
            std::string generated_code_string;
            for (const auto& switch_keys : permutation.keys())
                generated_code_string += std::format("export static const bool {} = {};", switch_keys, permutation.get(switch_keys) ? "true" : "false");

            // Load the specialization constant module from string.
            auto permutation_values_src = UnownedRawBlob::create(generated_code_string.c_str(), generated_code_string.size());
            components.emplace_back(session->loadModuleFromSource("permutation-values", "permutation-values.slang", permutation_values_src));
        }
        components.emplace_back(entry_point);
        Slang::ComPtr<slang::IComponentType> program;
        if (SLANG_FAILED(session->createCompositeComponentType(components.data(), components.size(), program.writeRef())))
            return result.push_error({"Failed to create stage program"});

        bool b_is_render_pass = false;
        auto global_var       = entry_point->getFunctionReflection();
        for (size_t at = 0; at < global_var->getUserAttributeCount(); ++at)
        {
            auto user_attribute = global_var->getUserAttributeByIndex(at);
            if (user_attribute->getName() != std::string("RenderPass"))
                continue;

            for (size_t ar = 0; ar < user_attribute->getArgumentCount(); ++ar)
            {
                size_t      size;
                const char* str = user_attribute->getArgumentValueString(ar, &size);
                if (std::string(str, size) == '"' + render_pass + '"')
                {
                    b_is_render_pass = true;
                    break;
                }
            }
            if (b_is_render_pass)
                break;
        }
        if (!b_is_render_pass)
            continue;

        std::cout << "\tMAIN = '" << entry_point->getFunctionReflection()->getName() << "':\n";
        for (SlangUInt j = 0; j < entry_point->getFunctionReflection()->getParameterCount(); j++)
        {
            auto param = entry_point->getFunctionReflection()->getParameterByIndex(j);
            std::cout << "\t\t- " << param->getType()->getName() << " " << param->getName() << "\n";
        }

        Slang::ComPtr<slang::IComponentType> linkedProgram;
        Slang::ComPtr<slang::IBlob>          diagnostics;
        program->link(linkedProgram.writeRef(), diagnostics.writeRef());
        if (diagnostics)
            return result.push_error({static_cast<const char*>(diagnostics->getBufferPointer())});

        Slang::ComPtr<slang::IBlob> kernel_blob;
        linkedProgram->getEntryPointCode(0, 0, kernel_blob.writeRef(), diagnostics.writeRef());

        std::vector<uint8_t> buffer(kernel_blob->getBufferSize());
        memcpy(buffer.data(), kernel_blob->getBufferPointer(), kernel_blob->getBufferSize());

        slang::IMetadata* metadata;
        linkedProgram->getEntryPointMetadata(0, 0, &metadata, diagnostics.writeRef());
        if (diagnostics)
            return result.push_error({static_cast<const char*>(diagnostics->getBufferPointer())});

        StageData             data;
        slang::ProgramLayout* shaderReflection = linkedProgram->getLayout();
        for (unsigned pp = 0; pp < shaderReflection->getParameterCount(); pp++)
        {
            bool                             b_is_used = false;
            slang::VariableLayoutReflection* parameter = shaderReflection->getParameterByIndex(pp);
            metadata->isParameterLocationUsed(static_cast<SlangParameterCategory>(parameter->getCategory()), 0, parameter->getBindingIndex(), b_is_used);
            if (b_is_used)
            {
                if (parameter->getCategory() == slang::PushConstantBuffer)
                {
                    data.push_constant_size = static_cast<uint32_t>(parameter->getTypeLayout()->getStride(SLANG_PARAMETER_CATEGORY_PUSH_CONSTANT_BUFFER));
                    std::cout << "pc size : " << data.push_constant_size << std::endl;
                }
                else if (parameter->getCategory() == slang::DescriptorTableSlot)
                {
                    Eng::Gfx::EBindingType binding_type;

                    if (parameter->getType()->getKind() == slang::TypeReflection::Kind::Resource)
                    {
                        auto shape = parameter->getType()->getResourceShape();

                        if (shape == SLANG_TEXTURE_1D || shape == SLANG_TEXTURE_2D || shape == SLANG_TEXTURE_3D || shape == SLANG_TEXTURE_CUBE)
                            binding_type = Eng::Gfx::EBindingType::SAMPLED_IMAGE;
                        else if (shape == SLANG_STRUCTURED_BUFFER)
                            binding_type = Eng::Gfx::EBindingType::STORAGE_BUFFER;
                        else
                            return result.push_error({"Unhandled descriptor parameter shape"});
                    }
                    else if (parameter->getType()->getKind() == slang::TypeReflection::Kind::SamplerState)
                        binding_type = Eng::Gfx::EBindingType::SAMPLER;
                    else
                        return result.push_error({"Unhandled descriptor parameter type"});

                    data.bindings.emplace_back(parameter->getName(), parameter->getBindingIndex(), binding_type);
                    std::cout << "\t-" << parameter->getName() << " : " << (int)binding_type << " binding=" << parameter->getBindingIndex() << "\n";
                }
                else
                    return result.push_error({"Unhandled parameter category"});
            }

            // BINDINGS HERE
        }

        Eng::Gfx::EShaderStage stage;

        switch (entry_point->getLayout()->getEntryPointByIndex(0)->getStage())
        {
        case SLANG_STAGE_VERTEX:
            stage = Eng::Gfx::EShaderStage::Vertex;
            break;
        case SLANG_STAGE_FRAGMENT:
            stage = Eng::Gfx::EShaderStage::Fragment;
            break;
        case SLANG_STAGE_COMPUTE:
            stage = Eng::Gfx::EShaderStage::Compute;
            break;
        default:
            return result.push_error({"Unhandled shader stage type"});
        }

        data.compiled_module = std::vector<uint8_t>(kernel_blob->getBufferSize());
        std::memcpy(data.compiled_module.data(), kernel_blob->getBufferPointer(), kernel_blob->getBufferSize());
        result.stages.emplace(stage, data);
    }
    return result;
}
} // namespace Eng::Gfx