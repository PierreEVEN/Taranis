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

TypeReflection::TypeReflection(slang::TypeReflection* slang_type)
{
    if (!can_reflect(slang_type))
        return;
    switch (slang_type->getKind())
    {
    case slang::TypeReflection::Kind::Struct:
        for (size_t i = 0; i < slang_type->getFieldCount(); ++i)
        {
            push_variable(slang_type->getFieldByIndex(static_cast<uint32_t>(i)));
        }
        break;
    case slang::TypeReflection::Kind::Vector:
    case slang::TypeReflection::Kind::Matrix:
        switch (slang_type->getScalarType())
        {
        case slang::TypeReflection::None:
        case slang::TypeReflection::Void:
            break;
        case slang::TypeReflection::Bool:
        case slang::TypeReflection::Int8:
        case slang::TypeReflection::UInt8:
            total_stride += slang_type->getElementCount() * 1;
            break;
        case slang::TypeReflection::Float16:
        case slang::TypeReflection::Int16:
        case slang::TypeReflection::UInt16:
            total_stride += slang_type->getElementCount() * 2;
            break;
        case slang::TypeReflection::Float32:
        case slang::TypeReflection::UInt32:
        case slang::TypeReflection::Int32:
            total_stride += slang_type->getElementCount() * 4;
            break;
        case slang::TypeReflection::Int64:
        case slang::TypeReflection::UInt64:
        case slang::TypeReflection::Float64:
            total_stride += slang_type->getElementCount() * 8;
            break;
        }
        break;
    }
}

void TypeReflection::push_variable(slang::VariableReflection* variable)
{
    if (can_reflect(variable->getType()))
        total_stride += children.emplace(variable->getName(), TypeReflection(variable->getType())).first->second.total_stride;
}

bool TypeReflection::can_reflect(slang::TypeReflection* slang_var)
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
    std::lock_guard   lk(session_lock);
    CompilationResult result;

    for (const auto& error : load_errors)
        result.push_error(error);
    if (!result.errors.empty())
        return result;

    for (SlangInt32 ep_i = 0; ep_i < module->getDefinedEntryPointCount(); ++ep_i)
    {
        Slang::ComPtr<slang::IEntryPoint> entry_point;
        if (SLANG_FAILED(module->getDefinedEntryPoint(ep_i, entry_point.writeRef())))
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
        for (uint32_t at_i = 0; at_i < global_var->getUserAttributeCount(); ++at_i)
        {
            auto user_attribute = global_var->getUserAttributeByIndex(at_i);
            if (user_attribute->getName() != std::string("RenderPass"))
                continue;

            for (uint32_t arg_i = 0; arg_i < user_attribute->getArgumentCount(); ++arg_i)
            {
                size_t      size;
                const char* str = user_attribute->getArgumentValueString(arg_i, &size);
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
        for (unsigned par_i = 0; par_i < shaderReflection->getParameterCount(); par_i++)
        {
            bool                             b_is_used = true;
            slang::VariableLayoutReflection* parameter = shaderReflection->getParameterByIndex(par_i);
            metadata->isParameterLocationUsed(static_cast<SlangParameterCategory>(parameter->getCategory()), 0, parameter->getBindingIndex(), b_is_used);

            if (parameter->getCategory() == slang::PushConstantBuffer && !b_is_used)
            {
                if (entry_point->getLayout()->getEntryPointByIndex(0)->getStage() == SLANG_STAGE_VERTEX)
                {
                    std::cout << std::format("UNUSED PC ISSUE !! {} : type={} / kind={} / cat={}\n", parameter->getName(), (int)parameter->getType()->getKind(),
                                             static_cast<uint32_t>(parameter->getTypeLayout()->getStride(SLANG_PARAMETER_CATEGORY_PUSH_CONSTANT_BUFFER)), static_cast<int>(parameter->getCategory()));
                    exit(-1);
                }
            }

            if (b_is_used)
            {
                if (parameter->getCategory() == slang::DescriptorTableSlot)
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

        for (unsigned par_i = 0; par_i < entry_point->getLayout()->getEntryPointByIndex(0)->getParameterCount(); ++par_i)
        {
            auto parameter = entry_point->getLayout()->getEntryPointByIndex(0)->getParameterByIndex(par_i);

            // @TODO Should be marked as PushConstant (slang issue : https://github.com/shader-slang/slang/issues/5676)
            if (parameter->getTypeLayout()->getParameterCategory() == slang::Uniform)
                data.push_constant_size = static_cast<uint32_t>(parameter->getTypeLayout()->getSize());
        }

        for (uint32_t pi = 0; pi < entry_point->getFunctionReflection()->getParameterCount(); ++pi)
            data.inputs.push_variable(entry_point->getFunctionReflection()->getParameterByIndex(pi));

        if (auto return_type = entry_point->getFunctionReflection()->getReturnType())
            data.outputs     = TypeReflection(return_type);

        data.compiled_module = std::vector<uint8_t>(kernel_blob->getBufferSize());
        std::memcpy(data.compiled_module.data(), kernel_blob->getBufferPointer(), kernel_blob->getBufferSize());
        result.stages.emplace(data.stage, data);
    }
    return result;
}
} // namespace Eng::Gfx