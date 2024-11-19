#include "shader_compiler/shader_compiler.hpp"

#include "llp/file_data.hpp"
#include "llp/lexical_analyzer.hpp"

#include <codecvt>

#include <Windows.h>
#include <d3d12shader.h>
#include <dxcapi.h>
#include <filesystem>

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

namespace Eng::Gfx
{
ShaderSource::ShaderSource(const std::string& raw)
{
    TextReader     text_reader(raw);
    TokenizerBlock code(text_reader);

    for (auto reader = code.read(); reader;)
    {
        switch (reader->type)
        {
        case TokenType::Symbol:
        {
            parse_errors.emplace_back(CompilationError{.error_message = "unexpected symbol", .line = static_cast<int64_t>(reader->line), .column = static_cast<int64_t>(reader->column)});
            return;
        }
        break;
        case TokenType::Word:
        {
            const std::string& word = *reader.consume<Word>(TokenType::Word);
            if (word == "option")
            {
                if (auto option_name = reader.consume<Word>(TokenType::Word))
                {
                    if (!reader.consume<Symbol>(TokenType::Symbol, '='))
                    {
                        parse_errors.emplace_back(CompilationError{.error_message = "Expected '='", .line = static_cast<int64_t>(reader->line), .column = static_cast<int64_t>(reader->column)});
                        return;
                    }

                    if (auto option_value = reader.consume<Word>(TokenType::Word))
                    {
                        if (*option_value == "true")
                            options.insert_or_assign(*option_name, true);
                        else if (*option_value == "false")
                            options.insert_or_assign(*option_name, false);
                        else
                        {
                            parse_errors.emplace_back(
                                CompilationError{.error_message = "Invalid option value, expected true or false", .line = static_cast<int64_t>(reader->line), .column = static_cast<int64_t>(reader->column)});
                            return;
                        }
                        if (!reader.consume<Symbol>(TokenType::Symbol, ';'))
                        {
                            parse_errors.emplace_back(CompilationError{.error_message = "; expected", .line = static_cast<int64_t>(reader->line), .column = static_cast<int64_t>(reader->column)});
                            return;
                        }
                    }
                }
                else
                {
                    parse_errors.emplace_back(CompilationError{.error_message = "Expected option name", .line = static_cast<int64_t>(reader->line), .column = static_cast<int64_t>(reader->column)});
                    return;
                }
            }
            else if (word == "pass")
            {
                if (auto pass_args = reader.consume<Arguments>(TokenType::Arguments))
                {
                    std::vector<RenderPass> passes;
                    for (const auto& tk : pass_args->tokens)
                    {
                        switch (tk->type)
                        {
                        case TokenType::Word:
                        {
                            passes.emplace_back(tk->data<Word>());
                        }
                        break;
                        case TokenType::Symbol:
                        {
                            {
                                if (tk->data<Symbol>() == ',')
                                    continue;
                                parse_errors.emplace_back(CompilationError{.error_message = "Unexpected symbol", .line = static_cast<int64_t>(reader->line)});
                                return;
                            }
                        }
                        break;
                        default:
                            parse_errors.emplace_back(CompilationError{.error_message = "unexpected token", .line = static_cast<int64_t>(reader->line), .column = static_cast<int64_t>(reader->column)});
                            return;
                        }
                    }

                    size_t start_location = reader->offset;
                    if (auto block = reader.consume<TokenizerBlock>(TokenType::TokenizerBlock))
                    {
                        RenderPassSources found_pass;

                        size_t end_location = reader ? reader->offset : UINT64_MAX;

                        found_pass.raw = raw.substr(start_location, end_location - start_location);

                        std::cout << " BLOCK = " << found_pass.raw << " " << start_location << " : " << end_location << std::endl;

                        for (auto& pass : passes)
                        {
                            std::cout << pass << "\n";
                            render_passes.insert_or_assign(pass, found_pass).first->second += found_pass;
                        }
                    }
                    else
                    {
                        parse_errors.emplace_back(CompilationError{.error_message = "Expected block", .line = static_cast<int64_t>(reader->line), .column = static_cast<int64_t>(reader->column)});
                        return;
                    }
                }
            }
        }
        break;
        case TokenType::Include:
        {
            reader.consume<Include>(TokenType::Include);
            parse_errors.emplace_back(CompilationError{.error_message = "Include directives are not supported", .line = static_cast<int64_t>(reader->line)});
            return;
        }
        break;
        case TokenType::TokenizerBlock:
        {
            reader.consume<TokenizerBlock>(TokenType::TokenizerBlock);
            parse_errors.emplace_back(CompilationError{.error_message = "unexpected block", .line = static_cast<int64_t>(reader->line), .column = static_cast<int64_t>(reader->column)});
            return;
        }
        break;
        case TokenType::Arguments:
        {
            reader.consume<Arguments>(TokenType::Arguments);
            parse_errors.emplace_back(CompilationError{.error_message = "unexpected arguments", .line = static_cast<int64_t>(reader->line), .column = static_cast<int64_t>(reader->column)});
            return;
        }
        break;
        case TokenType::Number:
        {
            reader.consume<Number>(TokenType::Number);
            parse_errors.emplace_back(CompilationError{.error_message = "unexpected number", .line = static_cast<int64_t>(reader->line), .column = static_cast<int64_t>(reader->column)});
            return;
        }
        break;
        }

    }
}

ShaderCompiler::ShaderCompiler()
{
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    utils->CreateDefaultIncludeHandler(&include_handler);
}

CompilationResult ShaderCompiler::compile_raw(const std::string& raw, const std::string& entry_point, EShaderStage shader_stage, const std::filesystem::path& path, std::vector<std::pair<std::string, bool>> options,
                                              bool               b_debug) const
{
    CompilationResult compilation_result;

    ShaderProperties result;
    result.entry_point = entry_point;
    result.stage       = shader_stage;

    std::wstring target_profile;
    switch (shader_stage)
    {
    case EShaderStage::Vertex:
        target_profile = L"vs_6_6";
        break;
    case EShaderStage::Fragment:
        target_profile = L"ps_6_6";
        break;
    default:
        compilation_result.errors.emplace_back(CompilationError{.error_message = "Unhandled shader stage"});
        return compilation_result;
    }
    std::wstring w_entry_point = to_u16string(entry_point);
    std::vector  arguments{
        L"-E", w_entry_point.c_str(), L"-T", target_profile.c_str(), DXC_ARG_PACK_MATRIX_ROW_MAJOR, DXC_ARG_WARNINGS_ARE_ERRORS, DXC_ARG_ALL_RESOURCES_BOUND, L"-spirv",
    };

    std::vector<std::wstring> features_w_string;

    for (const auto& feature : options)
    {
        if (!feature.second)
            continue;
        arguments.push_back(L"-D");
        features_w_string.push_back(to_u16string(feature.first));
        arguments.push_back(features_w_string.back().c_str());
    }

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
        .Ptr = sourceBlob->GetBufferPointer(),
        .Size = sourceBlob->GetBufferSize(),
        .Encoding = 0u,
    };

    // Compile the shader.
    Microsoft::WRL::ComPtr<IDxcResult> compiledShaderBuffer{};
    const HRESULT                      hr = compiler->Compile(&sourceBuffer, arguments.data(), static_cast<uint32_t>(arguments.size()), include_handler, IID_PPV_ARGS(&compiledShaderBuffer));

    if (FAILED(hr))
    {
        compilation_result.errors.emplace_back(CompilationError{.error_message = std::format("Failed to compile shader with path : {}", path.string())});
        return compilation_result;
    }

    // Get compilation errors (if any).
    Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors{};
    if (FAILED(compiledShaderBuffer->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr)))
    {
        compilation_result.errors.emplace_back(CompilationError{.error_message = std::format("Failed to get shader errors : {}", std::system_category().message(hr))});
        return compilation_result;
    }
    if (errors && errors->GetStringLength() > 0)
    {
        const LPCSTR errorMessage = errors->GetStringPointer();
        compilation_result.errors.emplace_back(CompilationError{.error_message = std::format("Failed to get dxc compiler output : {}", errorMessage)});
        return compilation_result;
    }

    Microsoft::WRL::ComPtr<IDxcBlob> compiledShaderBlob{nullptr};
    compiledShaderBuffer->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&compiledShaderBlob), nullptr);

    result.spirv.resize(compiledShaderBlob->GetBufferSize());
    memcpy(result.spirv.data(), compiledShaderBlob->GetBufferPointer(), compiledShaderBlob->GetBufferSize());

    if (auto reflection = extract_spirv_properties(result))
    {
        compilation_result.errors.emplace_back(CompilationError{.error_message = std::format("Failed to get shader reflection : {}", *reflection)});
        return compilation_result;
    }

    return compilation_result;
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
            if (static_cast<int>(var->built_in) >= 0)
                continue;
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
} // namespace Eng::Gfx