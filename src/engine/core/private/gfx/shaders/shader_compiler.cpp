#include "gfx/shaders/shader_compiler.hpp"

#include <fstream>
#include <dxcapi.h>


#include "logger.hpp"
#include "gfx/vulkan/shader_module.hpp"


#include <Windows.h>

namespace Engine
{
	ShaderCompiler::ShaderCompiler()
	{
		DxcCreateInstance(CLSID_DxcCompiler, &compiler);
	}

	Spirv ShaderCompiler::compile_raw(const std::string& raw, const std::string& entry_point, EShaderStage stage)
	{
		IDxcUtils* pUtils;
		DxcCreateInstance(CLSID_DxcUtils, &pUtils);
		IDxcBlobEncoding* pSource;
		pUtils->CreateBlob(raw.data(), raw.size(), CP_UTF8, &pSource);

		std::vector<const wchar_t*> arguments;
		// -E for the entry point (eg. 'main')
		arguments.push_back(L"-E");
		arguments.push_back(entry_point.c_str());

		// -T for the target profile (eg. 'ps_6_6')
		arguments.push_back(L"-T");
		switch (stage)
		{
		case EShaderStage::Vertex:
			arguments.push_back(L"vs_6.6");
			break;
		case EShaderStage::Fragment:
			arguments.push_back(L"ps_6.6");
			break;
		default:
			LOG_FATAL("Unhandled default case");
		}


		// Strip reflection data and pdbs (see later)
		arguments.push_back(L"-Qstrip_debug");
		arguments.push_back(L"-Qstrip_reflect");

		arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX
		arguments.push_back(DXC_ARG_DEBUG); //-Zi

		/*
		for (const std::wstring& define : defines)
		{
			arguments.push_back(L"-D");
			arguments.push_back(define.c_str());
		}
		*/

		DxcBuffer sourceBuffer;
		sourceBuffer.Ptr = pSource->GetBufferPointer();
		sourceBuffer.Size = pSource->GetBufferSize();
		sourceBuffer.Encoding = 0;

		IDxcResult* pCompileResult;
		compiler->Compile(&sourceBuffer, arguments.data(), static_cast<uint32_t>(arguments.size()), nullptr,
		                  (&pCompileResult));

		// Error Handling. Note that this will also include warnings unless disabled.
		IDxcBlobUtf8* pErrors;
		pCompileResult->GetOutput(DXC_OUT_ERRORS, (&pErrors), nullptr);
		if (pErrors && pErrors->GetStringLength() > 0)
		{
			LOG_FATAL("Failed to compile shader : {}", static_cast<const char*>(pErrors->GetBufferPointer()));
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
