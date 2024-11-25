#include "shader_compiler/shader_compiler.hpp"

#include "slang.h"
#include "slang-com-ptr.h"
#include "slang_helper.hpp"

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

std::shared_ptr<Session> Compiler::create_session(const std::filesystem::path& path)
{
    return std::shared_ptr<Session>(new Session(this, path));
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

    /*slang::ProgramLayout* layout = module->getLayout(0, diagnostics.writeRef());
    if (diagnostics)
    {
        load_errors.emplace_back(static_cast<const char*>(diagnostics->getBufferPointer()));
        return;
    }*/
}

Session::~Session()
{
    session->release();
}

InOutReflection::InOutReflection(slang::TypeReflection* slang_var)
{
    if (!can_reflect(slang_var))
        return;
    switch (slang_var->getKind())
    {
    case slang::TypeReflection::Kind::Struct:
        for (size_t i = 0; i < slang_var->getFieldCount(); ++i)
        {
            push_variable(slang_var->getFieldByIndex(static_cast<uint32_t>(i)));
        }
        break;
    case slang::TypeReflection::Kind::Vector:
    case slang::TypeReflection::Kind::Matrix:
        switch (slang_var->getScalarType())
        {
        case slang::TypeReflection::None:
        case slang::TypeReflection::Void:
            break;
        case slang::TypeReflection::Bool:
        case slang::TypeReflection::Int8:
        case slang::TypeReflection::UInt8:
            total_stride += slang_var->getElementCount() * 1;
            break;
        case slang::TypeReflection::Float16:
        case slang::TypeReflection::Int16:
        case slang::TypeReflection::UInt16:
            total_stride += slang_var->getElementCount() * 2;
            break;
        case slang::TypeReflection::Float32:
        case slang::TypeReflection::UInt32:
        case slang::TypeReflection::Int32:
            total_stride += slang_var->getElementCount() * 4;
            break;
        case slang::TypeReflection::Int64:
        case slang::TypeReflection::UInt64:
        case slang::TypeReflection::Float64:
            total_stride += slang_var->getElementCount() * 8;
            break;
        }
        break;
    }
}

void InOutReflection::push_variable(slang::VariableReflection* variable)
{
    if (can_reflect(variable->getType()))
        total_stride += children.emplace(variable->getName(), InOutReflection(variable->getType())).first->second.total_stride;
}

bool InOutReflection::can_reflect(slang::TypeReflection* slang_var)
{
    switch (slang_var->getKind())
    {
    case slang::TypeReflection::Kind::Struct:
        return true;
    case slang::TypeReflection::Kind::Vector:
    case slang::TypeReflection::Kind::Matrix:
        return slang_var->getElementType()->getKind() == slang::TypeReflection::Kind::Scalar;
    default:
        return false;
    }
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
        for (uint32_t at = 0; at < global_var->getUserAttributeCount(); ++at)
        {
            auto user_attribute = global_var->getUserAttributeByIndex(at);
            if (user_attribute->getName() != std::string("RenderPass"))
                continue;

            for (uint32_t ar = 0; ar < user_attribute->getArgumentCount(); ++ar)
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
        for (uint32_t j = 0; j < entry_point->getFunctionReflection()->getParameterCount(); j++)
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

        switch (entry_point->getLayout()->getEntryPointByIndex(0)->getStage())
        {
        case SLANG_STAGE_VERTEX:
            data.stage = Eng::Gfx::EShaderStage::Vertex;
            break;
        case SLANG_STAGE_FRAGMENT:
            data.stage = Eng::Gfx::EShaderStage::Fragment;
            break;
        case SLANG_STAGE_COMPUTE:
            data.stage = Eng::Gfx::EShaderStage::Compute;
            break;
        default:
            return result.push_error({"Unhandled shader stage type"});
        }

        for (uint32_t pi = 0; pi < entry_point->getFunctionReflection()->getParameterCount(); ++pi)
            data.inputs.push_variable(entry_point->getFunctionReflection()->getParameterByIndex(pi));

        if (auto return_type = entry_point->getFunctionReflection()->getReturnType())
            data.outputs     = InOutReflection(return_type);

        data.compiled_module = std::vector<uint8_t>(kernel_blob->getBufferSize());
        std::memcpy(data.compiled_module.data(), kernel_blob->getBufferPointer(), kernel_blob->getBufferSize());
        result.stages.emplace(data.stage, data);
    }
    return result;
}
} // namespace Eng::Gfx