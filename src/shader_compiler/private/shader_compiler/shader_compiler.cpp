#include "shader_compiler/shader_compiler.hpp"

#include "llp/file_data.hpp"
#include "shader_compiler/shader_parser.hpp"

#include <codecvt>

#include <Windows.h>
#include <d3d12shader.h>
#include <dxcapi.h>
#include <filesystem>
#include <ranges>

#include <spirv_reflect.h>
#include <wrl/client.h>

#pragma warning(push)
#pragma warning(disable : 4996)
static std::wstring to_u16string(const std::string& str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}
#pragma warning(pop)

namespace ShaderCompiler
{

Compiler::Compiler()
{
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    utils->CreateDefaultIncludeHandler(&include_handler);
}

std::optional<CompilationError> Compiler::compile_raw(const ShaderParser& parser, const Parameters& params, CompilationResult& compilation_result) const
{
    if (params.selected_passes.empty())
    {
        for (const auto& pass : parser.get_passes() | std::views::keys)
            if (auto error = compile_pass(pass, parser, params, compilation_result))
                return error;
    }
    else
    {
        for (const auto& pass : params.selected_passes)
            if (auto error = compile_pass(pass, parser, params, compilation_result))
                return error;
    }
    return {};
}

std::optional<CompilationError> Compiler::compile_pass(const std::string& pass_name, const ShaderParser& parser, const Parameters& params, CompilationResult& compilation_result) const
{
    if (auto pass = parser.get_passes().find(pass_name); pass != parser.get_passes().end())
    {
        std::vector<EntryPoint> entry_points;

        std::string code;
        for (const auto& block : pass->second)
        {
            code += block->raw_code;
            for (const auto& entry_point : block->entry_point)
                entry_points.emplace_back(entry_point);
        }

        CompiledPass pass_data;
        pass_data.pipeline_config = parser.get_pipeline_config();
        for (const auto& entry_point : entry_points)
        {
            CompiledStage result;
            if (auto compilation_error = compile_stage(entry_point.stage, code, entry_point.name, params, result))
                return compilation_error;
            pass_data.stages.emplace(entry_point.stage, result);
        }
        compilation_result.compiled_passes.emplace(pass->first, pass_data);
    }
    return {};
}

std::optional<CompilationError> Compiler::compile_stage(Eng::Gfx::EShaderStage stage, const std::string& code, const std::string& entry_point, const Parameters& params, CompiledStage& result) const
{
    result.entry_point = entry_point;
    result.stage       = stage;

    std::wstring target_profile;
    switch (stage)
    {
    case Eng::Gfx::EShaderStage::Vertex:
        target_profile = L"vs_6_6";
        break;
    case Eng::Gfx::EShaderStage::Fragment:
        target_profile = L"ps_6_6";
        break;
    default:
        return CompilationError{"unhandled shader stage"};
    }
    std::wstring w_entry_point = to_u16string(entry_point);
    std::vector  arguments{
        L"-E", w_entry_point.c_str(), L"-T", target_profile.c_str(), DXC_ARG_PACK_MATRIX_ROW_MAJOR, DXC_ARG_WARNINGS_ARE_ERRORS, DXC_ARG_ALL_RESOURCES_BOUND, L"-spirv",
    };

    std::vector<std::wstring> features_w_string;

    for (const auto& feature : params.options_override)
    {
        if (!feature.second)
            continue;
        arguments.push_back(L"-D");
        features_w_string.push_back(to_u16string(feature.first));
        arguments.push_back(features_w_string.back().c_str());
    }

    // Indicate that the shader should be in a debuggable state if in debug mode.
    // Else, set optimization level to 3.
    if (params.b_debug)
        arguments.push_back(DXC_ARG_DEBUG);
    else
        arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);

    // Load the shader source file to a blob.
    Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceBlob{};
    utils->CreateBlob(code.data(), static_cast<UINT32>(code.size()), DXC_CP_UTF8, &sourceBlob);

    DxcBuffer sourceBuffer{
        .Ptr = sourceBlob->GetBufferPointer(),
        .Size = sourceBlob->GetBufferSize(),
        .Encoding = 0u,
    };

    // Compile the shader.
    Microsoft::WRL::ComPtr<IDxcResult> compiledShaderBuffer{};
    const HRESULT                      hr = compiler->Compile(&sourceBuffer, arguments.data(), static_cast<uint32_t>(arguments.size()), include_handler, IID_PPV_ARGS(&compiledShaderBuffer));

    if (FAILED(hr))
        return CompilationError{params.source_path ? std::format("Failed to compile shader with path : {}", params.source_path->string()) : "Failed to compile shader"};

    // Get compilation errors (if any).
    Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors{};
    if (FAILED(compiledShaderBuffer->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr)))
        return CompilationError{std::format("Failed to get shader errors : {}", std::system_category().message(hr))};
    if (errors && errors->GetStringLength() > 0)
    {
        const LPCSTR errorMessage = errors->GetStringPointer();
        return CompilationError{std::format("Failed to get dxc compiler output : {}", errorMessage)};
    }
    Microsoft::WRL::ComPtr<IDxcBlob> compiledShaderBlob{nullptr};
    compiledShaderBuffer->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&compiledShaderBlob), nullptr);

    result.spirv.resize(compiledShaderBlob->GetBufferSize());
    memcpy(result.spirv.data(), compiledShaderBlob->GetBufferPointer(), compiledShaderBlob->GetBufferSize());

    if (auto reflection_error = extract_spirv_properties(result))
        return reflection_error;
    return {};
}

std::optional<CompilationError> Compiler::extract_spirv_properties(CompiledStage& properties)
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
            if (static_cast<int>(var->built_in) >= 0)
                continue;
            properties.stage_inputs.emplace_back(var->location, (var->word_offset.location - first_offset) * 2, static_cast<Eng::Gfx::ColorFormat>(var->format));
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
            return std::format("Unhandled descriptor type : {}", static_cast<int>(binding->descriptor_type));
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
            properties.stage_outputs.emplace_back(var->location, (var->word_offset.location - first_offset) * 2, static_cast<Eng::Gfx::ColorFormat>(var->format));
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
            return CompilationError{"multiple push constants are not supported"};
        properties.push_constant_size = push_constants[0]->size;
    }

    // Output variables, descriptor bindings, descriptor sets, and push constants
    // can be enumerated and extracted using a similar mechanism.

    // Destroy the reflection data when no longer required.
    spvReflectDestroyShaderModule(&module);
    return {};
}
} // namespace Eng::Gfx