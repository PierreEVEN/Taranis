#include "gfx/shaders/shader_compiler.hpp"

#include <codecvt>
#include <fstream>

#include <Windows.h>
#include <d3d12shader.h>
#include <dxcapi.h>
#include <ranges>

#include "gfx/vulkan/shader_module.hpp"
#include "logger.hpp"

#include <spirv_reflect.h>
#include <wrl/client.h>

#include "gfx/types.hpp"

#include "gfx/vulkan/vk_check.hpp"

#pragma warning(push)
#pragma warning(disable : 4996)
static std::wstring to_u16string(const std::string& str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}
#pragma warning(pop)

void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        std::string message = std::system_category().message(hr);
        LOG_FATAL("{}", message)
    }
}

namespace Engine::Gfx
{
ShaderCompiler::ShaderCompiler()
{
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    utils->CreateDefaultIncludeHandler(&include_handler);
}

Result<ShaderProperties> ShaderCompiler::compile_raw(const std::string& raw, const std::string& entry_point, EShaderStage stage, const std::filesystem::path& path, bool b_debug) const
{
    ShaderProperties result;
    result.entry_point = entry_point;
    result.stage       = stage;

    std::wstring target_profile;
    switch (stage)
    {
    case EShaderStage::Vertex:
        target_profile = L"vs_6_6";
        break;
    case EShaderStage::Fragment:
        target_profile = L"ps_6_6";
        break;
    default:
        LOG_FATAL("Unhandled shader stage")
    }
    std::wstring w_entry_point = to_u16string(entry_point);
    std::vector  arguments{
        L"-E", w_entry_point.c_str(), L"-T", target_profile.c_str(), DXC_ARG_PACK_MATRIX_ROW_MAJOR, DXC_ARG_WARNINGS_ARE_ERRORS, DXC_ARG_ALL_RESOURCES_BOUND, L"-spirv",
    };
    // Indicate that the shader should be in a debuggable state if in debug mode.
    // Else, set optimization level to 3.
    if (b_debug)
        arguments.push_back(DXC_ARG_DEBUG);
    else
        arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);

    // Load the shader source file to a blob.
    Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceBlob{};
    utils->CreateBlob(raw.data(), static_cast<UINT32>(raw.size()), DXC_CP_UTF8, &sourceBlob);

    DxcBuffer sourceBuffer{
        .Ptr      = sourceBlob->GetBufferPointer(),
        .Size     = sourceBlob->GetBufferSize(),
        .Encoding = 0u,
    };

    // Compile the shader.
    Microsoft::WRL::ComPtr<IDxcResult> compiledShaderBuffer{};
    const HRESULT                      hr = compiler->Compile(&sourceBuffer, arguments.data(), static_cast<uint32_t>(arguments.size()), include_handler, IID_PPV_ARGS(&compiledShaderBuffer));

    if (FAILED(hr))
        return Result<ShaderProperties>::Error(std::format("Failed to compile shader with path : {}", path.string()));

    // Get compilation errors (if any).
    Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors{};
    ThrowIfFailed(compiledShaderBuffer->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));
    if (errors && errors->GetStringLength() > 0)
    {
        const LPCSTR errorMessage = errors->GetStringPointer();
        return Result<ShaderProperties>::Error(std::format("{}", errorMessage));
    }

    Microsoft::WRL::ComPtr<IDxcBlob> compiledShaderBlob{nullptr};
    compiledShaderBuffer->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&compiledShaderBlob), nullptr);

    result.spirv.resize(compiledShaderBlob->GetBufferSize());
    memcpy(result.spirv.data(), compiledShaderBlob->GetBufferPointer(), compiledShaderBlob->GetBufferSize());

    if (auto reflection = extract_spirv_properties(result))
        return Result<ShaderProperties>::Error(*reflection);

    return Result<ShaderProperties>::Ok(result);
}

Result<ShaderProperties> ShaderCompiler::load_from_path(const std::filesystem::path& path, const std::string& entry_point, EShaderStage stage, bool b_debug) const
{
    std::string   shader_code;
    std::ifstream shader_file(path);
    if (shader_file.is_open())
    {
        while (shader_file)
        {
            std::string line;
            std::getline(shader_file, line);
            shader_code += line + "\n";
        }
    }
    return compile_raw(shader_code, entry_point, stage, path, b_debug);
}

std::optional<std::string> ShaderCompiler::extract_spirv_properties(ShaderProperties& properties)
{
    // Generate reflection data for a shader
    SpvReflectShaderModule module;
    SpvReflectResult       result = spvReflectCreateShaderModule(properties.spirv.size(), properties.spirv.data(), &module);

    if (result != SPV_REFLECT_RESULT_SUCCESS)
        return std::format("Failed to get spirv reflection : Code {}", static_cast<uint32_t>(result));

    // Enumerate and extract shader's input variables
    uint32_t var_count = 0;
    result             = spvReflectEnumerateEntryPointInputVariables(&module, properties.entry_point.c_str(), &var_count, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        return std::format("Failed to enumerate spirv input variables count : Code {}", static_cast<uint32_t>(result));
    std::vector<SpvReflectInterfaceVariable*> input_vars(var_count, nullptr);
    result = spvReflectEnumerateEntryPointInputVariables(&module, properties.entry_point.c_str(), &var_count, input_vars.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        return std::format("Failed to enumerate spirv input variables values : Code {}", static_cast<uint32_t>(result));

    if (!input_vars.empty())
    {
        uint32_t first_offset = input_vars[0]->word_offset.location;
        for (const auto& var : input_vars)
        {
            properties.stage_inputs.emplace_back(var->location, (var->word_offset.location - first_offset) * 2, static_cast<ColorFormat>(var->format));
        }
    }

    // Enumerate and extract shader's bindings
    result = spvReflectEnumerateEntryPointDescriptorBindings(&module, properties.entry_point.c_str(), &var_count, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        return std::format("Failed to enumerate spirv bindings count : Code {}", static_cast<uint32_t>(result));
    std::vector<SpvReflectDescriptorBinding*> descriptor_bindings(var_count, nullptr);
    result = spvReflectEnumerateEntryPointDescriptorBindings(&module, properties.entry_point.c_str(), &var_count, descriptor_bindings.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        return std::format("Failed to enumerate spirv bindings values : Code {}", static_cast<uint32_t>(result));

    for (const auto& binding : descriptor_bindings)
    {
        EBindingType binding_type;
        switch (binding->descriptor_type)
        {
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
            binding_type = EBindingType::SAMPLER;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            binding_type = EBindingType::COMBINED_IMAGE_SAMPLER;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            binding_type = EBindingType::SAMPLED_IMAGE;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            binding_type = EBindingType::STORAGE_IMAGE;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            binding_type = EBindingType::UNIFORM_TEXEL_BUFFER;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            binding_type = EBindingType::STORAGE_TEXEL_BUFFER;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            binding_type = EBindingType::UNIFORM_BUFFER;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            binding_type = EBindingType::STORAGE_BUFFER;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            binding_type = EBindingType::UNIFORM_BUFFER_DYNAMIC;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            binding_type = EBindingType::STORAGE_BUFFER_DYNAMIC;
            break;
        case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            binding_type = EBindingType::INPUT_ATTACHMENT;
            break;
        default:
            LOG_FATAL("Unhandled descriptor type : {}", static_cast<int>(binding->descriptor_type))
        }
        properties.bindings.emplace_back(binding->name, binding->binding, binding_type);
    }

    // Enumerate and extract shader's output
    result = spvReflectEnumerateEntryPointOutputVariables(&module, properties.entry_point.c_str(), &var_count, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        return std::format("Failed to enumerate spirv output variables count : Code {}", static_cast<uint32_t>(result));
    std::vector<SpvReflectInterfaceVariable*> output_variables(var_count, nullptr);
    result = spvReflectEnumerateEntryPointOutputVariables(&module, properties.entry_point.c_str(), &var_count, output_variables.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        return std::format("Failed to enumerate spirv output variables values : Code {}", static_cast<uint32_t>(result));

    if (!output_variables.empty())
    {
        uint32_t first_offset = output_variables[0]->word_offset.location;
        for (const auto& var : output_variables)
            properties.stage_outputs.emplace_back(var->location, (var->word_offset.location - first_offset) * 2, static_cast<ColorFormat>(var->format));
    }

    // Enumerate and extract push constants
    result = spvReflectEnumerateEntryPointPushConstantBlocks(&module, properties.entry_point.c_str(), &var_count, nullptr);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        return std::format("Failed to enumerate spirv output push constants count : Code {}", static_cast<uint32_t>(result));
    std::vector<SpvReflectBlockVariable*> push_constants(var_count, nullptr);
    result = spvReflectEnumerateEntryPointPushConstantBlocks(&module, properties.entry_point.c_str(), &var_count, push_constants.data());
    if (result != SPV_REFLECT_RESULT_SUCCESS)
        return std::format("Failed to enumerate spirv output push constants values : Code {}", static_cast<uint32_t>(result));

    if (!push_constants.empty())
    {
        if (push_constants.size() != 1)
            return "multiple push constants are not supported";
        properties.push_constant_size = push_constants[0]->size;
    }

    // Output variables, descriptor bindings, descriptor sets, and push constants
    // can be enumerated and extracted using a similar mechanism.

    // Destroy the reflection data when no longer required.
    spvReflectDestroyShaderModule(&module);
    return {};
}
} // namespace Engine
