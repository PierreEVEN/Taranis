#include "gfx/shaders/shader_compiler.hpp"

#include <codecvt>
#include <fstream>
#include <dxcapi.h>


#include "logger.hpp"
#include "gfx/vulkan/shader_module.hpp"


#include <Windows.h>

static std::wstring to_u16string(const std::string& str) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(str);

}

namespace Engine
{
	ShaderCompiler::ShaderCompiler()
	{
		DxcCreateInstance(CLSID_DxcCompiler, &compiler);
	}

	Spirv ShaderCompiler::compile_raw(const std::string& raw, const std::string& entry_point, EShaderStage stage)
	{
		IDxcUtils* utils;
		DxcCreateInstance(CLSID_DxcUtils, &pUtils);
		IDxcBlobEncoding* pSource;

		::DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		::DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
		//utils->CreateDefaultIncludeHandler(&includeHandler);

		std::wstring target_profile;
		switch (stage)
		{
		case EShaderStage::Vertex:
			target_profile = L"vs_6.6";
			break;
		case EShaderStage::Fragment:
			target_profile = L"ps_6.6";
			break;
		default:
			LOG_FATAL("Unhandled default case");
		}

		std::wstring w_entry_point = to_u16string(entry_point);

		std::vector<LPCWSTR> arguments{
			L"-E",
			w_entry_point.c_str(),
			L"-T",
			(target_profile.c_str()),
			DXC_ARG_PACK_MATRIX_ROW_MAJOR,
			DXC_ARG_WARNINGS_ARE_ERRORS,
			DXC_ARG_ALL_RESOURCES_BOUND,
		};
		// Indicate that the shader should be in a debuggable state if in debug mode.
// Else, set optimization level to 3.
		if constexpr (DEBUG_MODE)
		{
			arguments.push_back(DXC_ARG_DEBUG);
		}
		else
		{
			arguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
		}



		// Load the shader source file to a blob.
		IDxcBlobEncoding* sourceBlob{};
		utils->LoadFile(shaderPath.data(), nullptr, &sourceBlob);

		DxcBuffer sourceBuffer
		{
			.Ptr = sourceBlob->GetBufferPointer(),
			.Size = sourceBlob->GetBufferSize(),
			.Encoding = 0u,
		};

		// Compile the shader.
		IDxcResult* compiledShaderBuffer{};
		const HRESULT hr = compiler->Compile(&sourceBuffer,
			compilationArguments.data(),
			static_cast<uint32_t>(compilationArguments.size()),
			includeHandler.Get(),
			IID_PPV_ARGS(&compiledShaderBuffer));
		if (FAILED(hr))
		{
			LOG_FATAL(std::wstring(L"Failed to compile shader with path : ") + shaderPath.data());
		}

		// Get compilation errors (if any).
		IDxcBlobUtf8* errors{};
		compiledShaderBuffer->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength() > 0)
		{
			const LPCSTR errorMessage = errors->GetStringPointer();
			LOG_FATAL(errorMessage);
		}
	}

	Spirv ShaderCompiler::load_from_path(const std::filesystem::path& path, const std::string& entry_point,
	                                     EShaderStage stage)
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
		return compile_raw(shader_code, entry_point, stage);
	}
}
