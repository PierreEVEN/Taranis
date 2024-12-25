#include "shader_compiler/shader_compiler.hpp"

#include "logger.hpp"
#include "slang-com-ptr.h"
#include "slang.h"
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
    std::lock_guard    lk(session_lock);
    std::lock_guard    lk2(in_compiler->global_session_lock);
    slang::SessionDesc sessionDesc;

    // Target
    slang::TargetDesc targetDesc;
    targetDesc.format  = SLANG_SPIRV;
    targetDesc.profile = compiler->global_session->findProfile("spirv_1_5");
    if (targetDesc.profile == SLANG_PROFILE_UNKNOWN)
    {
        load_errors.emplace_back("Failed to find slang profile 'spirv_1_5'");
        return;
    }
    sessionDesc.targets                 = &targetDesc;
    sessionDesc.targetCount             = 1;
    sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

    // Search paths
    const char* searchPaths[]   = {"resources/shaders"};
    sessionDesc.searchPaths     = searchPaths;
    sessionDesc.searchPathCount = 1;

    std::vector<slang::CompilerOptionEntry> compiler_options;
    //compiler_options.emplace_back(slang::CompilerOptionEntry{slang::CompilerOptionName::UseUpToDateBinaryModule, slang::CompilerOptionValue{.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1}});
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

        auto moduleReflection = module->getModuleReflection();
    for (auto child : moduleReflection->getChildren())
    {
        auto type = child->getKind();
        if (type == slang::DeclReflection::Kind::Variable)
        {
            auto asVariable = child->asVariable();
            bool hasExtern  = asVariable->findModifier(slang::Modifier::Extern) != nullptr;
            bool hasStatic  = asVariable->findModifier(slang::Modifier::Static) != nullptr;
            bool hasConst   = asVariable->findModifier(slang::Modifier::Const) != nullptr;
            if (hasExtern && hasStatic && hasConst && static_cast<SlangScalarType>(asVariable->getType()->getScalarType()) == SLANG_SCALAR_TYPE_BOOL)
            {
                if (asVariable->hasDefaultValue())
                {
                    load_errors.emplace_back(std::format("{} : Default values are not supported for static switches. Please use [DefaultEnabled] user attribute instead", child->getName()));
                    break;
                }
                bool perm_value = false;
                for (uint32_t at_i = 0; at_i < asVariable->getUserAttributeCount(); ++at_i)
                {
                    auto user_attribute = asVariable->getUserAttributeByIndex(at_i);
                    if (user_attribute->getName() == std::string("DefaultEnabled"))
                    {
                        perm_value = true;
                        break;
                    }
                }
                permutation_description.permutation_group.emplace(asVariable->getName(), perm_value);
            }
        }
    }
}

std::optional<std::string> Session::try_register_variable(slang::VariableLayoutReflection* variable, ankerl::unordered_dense::map<std::string, StageInputOutputDescription>& in_outs, slang::IMetadata* metadata)
{
    if (variable->getTypeLayout()->getKind() == slang::TypeReflection::Kind::Struct)
    {
        for (uint32_t fi = 0; fi < variable->getTypeLayout()->getFieldCount(); ++fi)
            if (auto error = try_register_variable(variable->getTypeLayout()->getFieldByIndex(fi), in_outs, metadata))
                return error;
    }
    else
    {
        if (variable->getTypeLayout()->getBindingRangeCount() == 1)
        {
            bool b_used = true;
            metadata->isParameterLocationUsed(static_cast<SlangParameterCategory>(variable->getCategory()), variable->getBindingSpace(), variable->getBindingIndex(), b_used);
            if (b_used)
            {
                if (in_outs.contains(variable->getName()))
                    return "Duplicated input : " + std::string(variable->getName());
                in_outs.emplace(variable->getName(), StageInputOutputDescription{variable->getBindingIndex(), 0, Eng::Gfx::ColorFormat::UNDEFINED});
            }
        }
    }
    return {};
}

Session::~Session()
{
    session->release();
}

std::optional<std::filesystem::path> Session::get_filesystem_path() const
{
    if (!load_errors.empty())
        return {};
    return module->getFilePath() ? module->getFilePath() : std::optional<std::filesystem::path>{};
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


CompilationResult Session::compile(const std::string& render_pass, const Eng::Gfx::PermutationDescription& permutation)
{
    std::lock_guard   lk(session_lock);
    CompilationResult result;

    for (const auto& error : load_errors)
        result.push_error(error);
    if (!result.errors.empty())
        return result;

    ankerl::unordered_dense::map<std::string, bool> static_switches_values;
    for (const auto& key : permutation.keys())
        static_switches_values.emplace(key, permutation.get(key));
    for (const auto& key : permutation_description.permutation_group)
        static_switches_values.emplace(key.first, key.second);



    for (SlangInt32 ep_i = 0; ep_i < module->getDefinedEntryPointCount(); ++ep_i)
    {
        Slang::ComPtr<slang::IEntryPoint> entry_point;
        if (SLANG_FAILED(module->getDefinedEntryPoint(ep_i, entry_point.writeRef())))
            return result.push_error({"Failed to get entry point"});

        std::vector<slang::IComponentType*> components;
        if (!static_switches_values.empty())
        {
            std::string generated_code_string;
            for (const auto& switch_keys : static_switches_values)
                generated_code_string += std::format("export static const bool {} = {};", switch_keys.first, switch_keys.second ? "true" : "false");

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

        slang::IMetadata* metadata;
        linkedProgram->getEntryPointMetadata(0, 0, &metadata, diagnostics.writeRef());
        if (diagnostics)
            return result.push_error({static_cast<const char*>(diagnostics->getBufferPointer())});

        StageData data;

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

                    auto kind = parameter->getType()->getKind();

                    uint32_t array_element_count = 0;
                    if (parameter->getType()->getKind() == slang::TypeReflection::Kind::Array)
                    {
                        array_element_count = parameter->getType()->getElementCount() > 0 ? static_cast<uint32_t>(parameter->getType()->getElementCount()) : UINT32_MAX;
                        kind                = parameter->getType()->getElementType()->getKind();
                    }
                    if (kind == slang::TypeReflection::Kind::Resource)
                    {
                        auto shape = parameter->getType()->getResourceShape();

                        if (shape == SLANG_TEXTURE_1D || shape == SLANG_TEXTURE_2D || shape == SLANG_TEXTURE_3D || shape == SLANG_TEXTURE_CUBE)
                        {
                            if (parameter->getType()->getResourceAccess() == SLANG_RESOURCE_ACCESS_READ_WRITE)
                                binding_type = Eng::Gfx::EBindingType::STORAGE_IMAGE;
                            else
                                binding_type = Eng::Gfx::EBindingType::SAMPLED_IMAGE;
                        }
                        else if (shape == SLANG_STRUCTURED_BUFFER || shape == SLANG_BYTE_ADDRESS_BUFFER)
                            binding_type = Eng::Gfx::EBindingType::STORAGE_BUFFER;
                        else
                            return result.push_error({"Unhandled descriptor parameter shape " + std::to_string(static_cast<int>(shape)) + " for parameter " + parameter->getName()});
                    }
                    else if (kind == slang::TypeReflection::Kind::SamplerState)
                        binding_type = Eng::Gfx::EBindingType::SAMPLER;
                    else
                        return result.push_error({"Unhandled descriptor parameter type " + std::to_string(static_cast<int>(kind))});

                    data.bindings.emplace_back(parameter->getName(), parameter->getBindingIndex(), binding_type, array_element_count);
                }
                else
                    return result.push_error({"Unhandled parameter category"});
            }
        }

        for (unsigned par_i = 0; par_i < entry_point->getLayout()->getEntryPointByIndex(0)->getParameterCount(); ++par_i)
        {
            auto parameter = entry_point->getLayout()->getEntryPointByIndex(0)->getParameterByIndex(par_i);

            // Note : uniforms in stage parameters are considered as push constant in vulkan ecosystem
            if (parameter->getTypeLayout()->getParameterCategory() == slang::Uniform)
                data.push_constant_size = static_cast<uint32_t>(parameter->getTypeLayout()->getSize());
        }

        for (uint32_t pi = 0; pi < entry_point->getLayout()->getEntryPointByIndex(0)->getParameterCount(); ++pi)
        {
            auto parameter = entry_point->getLayout()->getEntryPointByIndex(0)->getParameterByIndex(pi);
            if (auto error = try_register_variable(parameter, data.inputs, metadata))
                return result.push_error({"Unhandled shader stage type"});
        }

        Slang::ComPtr<slang::IBlob> kernel_blob;
        linkedProgram->getEntryPointCode(0, 0, kernel_blob.writeRef(), diagnostics.writeRef());

        data.compiled_module = std::vector<uint8_t>(kernel_blob->getBufferSize());
        std::memcpy(data.compiled_module.data(), kernel_blob->getBufferPointer(), kernel_blob->getBufferSize());
        result.stages.emplace(data.stage, data);
    }
    return result;
}

Eng::Gfx::PermutationDescription Session::get_default_permutations_description() const
{
    return {permutation_description};
}
} // namespace ShaderCompiler