#include "gfx/shaders/shader_compiler.hpp"

#include <codecvt>
#include <fstream>

#include <Windows.h>
#include <dxcapi.h>
#include <d3d12shader.h>
#include <ranges>
#include <unordered_map>


#include "logger.hpp"
#include "gfx/vulkan/shader_module.hpp"


#include <wrl/client.h>


#include "gfx/types.hpp"
#include "gfx/vulkan/pipeline.hpp"

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
		LOG_FATAL("{}", message);
	}
}

namespace Engine
{
	ShaderCompiler::ShaderCompiler()
	{
		DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
		utils->CreateDefaultIncludeHandler(&include_handler);
	}

	Result<ShaderProperties> ShaderCompiler::compile_raw(const std::string& raw, const std::string& entry_point,
	                                                     EShaderStage stage, const std::filesystem::path& path,
	                                                     bool b_debug)
	{
		ShaderProperties result;
		result.entry_point = entry_point;
		result.stage = stage;

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
			LOG_FATAL("Unhandled shader stage");
		}
		std::wstring w_entry_point = to_u16string(entry_point);
		std::vector arguments{
			L"-E",
			w_entry_point.c_str(),
			L"-T",
			target_profile.c_str(),
			//DXC_ARG_PACK_MATRIX_ROW_MAJOR,
			//DXC_ARG_WARNINGS_ARE_ERRORS,
			//DXC_ARG_ALL_RESOURCES_BOUND,
			L"-spirv",
		};
		// Indicate that the shader should be in a debuggable state if in debug mode.
		// Else, set optimization level to 3.
		/*if (b_debug)
			arguments.push_back(DXC_ARG_DEBUG);
		else
			arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);*/


		// Load the shader source file to a blob.
		Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceBlob{};
		utils->CreateBlob(raw.data(), static_cast<UINT32>(raw.size()), DXC_CP_UTF8, &sourceBlob);

		DxcBuffer sourceBuffer
		{
			.Ptr = sourceBlob->GetBufferPointer(),
			.Size = sourceBlob->GetBufferSize(),
			.Encoding = 0u,
		};

		// Compile the shader.
		Microsoft::WRL::ComPtr<IDxcResult> compiledShaderBuffer{};
		const HRESULT hr = compiler->Compile(&sourceBuffer,
		                                     arguments.data(),
		                                     static_cast<uint32_t>(arguments.size()),
		                                     include_handler,
		                                     IID_PPV_ARGS(&compiledShaderBuffer));

		if (FAILED(hr))
			return Result<ShaderProperties>::Error(
				std::format("Failed to compile shader with path : {}", path.string()));

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


		result.spirv.resize(compiledShaderBlob->GetBufferSize() / 4 + (compiledShaderBlob->GetBufferSize() % 4 == 0 ? 0 : 1));
		memcpy(result.spirv.data(), compiledShaderBlob->GetBufferPointer(), compiledShaderBlob->GetBufferSize());

		return Result<ShaderProperties>::Ok(result);
	}

	Result<ShaderProperties> ShaderCompiler::load_from_path(const std::filesystem::path& path,
	                                                        const std::string& entry_point,
	                                                        EShaderStage stage, bool b_debug)
	{
		std::string shader_code;
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
}
