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
		// Set a breakpoint on this line to catch DirectX API errors
		throw std::exception();
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

	Result<Spirv> ShaderCompiler::compile_raw(const std::string& raw, const std::string& entry_point,
	                                          EShaderStage stage, const std::filesystem::path& path, bool b_debug)
	{
		std::vector<ShaderModule::Bindings> bindings;
		ShaderModule::CreateInfos in_create_infos;
		std::vector<Pipeline::VertexInput> vertex_inputs;

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
			LOG_FATAL("Unhandled default case");
		}
		std::wstring w_entry_point = to_u16string(entry_point);
		std::vector arguments{
			L"-E",
			w_entry_point.c_str(),
			L"-T",
			target_profile.c_str(),
			L"spirv",
			DXC_ARG_PACK_MATRIX_ROW_MAJOR,
			DXC_ARG_WARNINGS_ARE_ERRORS,
			DXC_ARG_ALL_RESOURCES_BOUND,
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
		{
			return Result<Spirv>::Error(std::format("Failed to compile shader with path : {}", path.string()));
		}

		// Get compilation errors (if any).
		Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors{};
		ThrowIfFailed(compiledShaderBuffer->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));
		if (errors && errors->GetStringLength() > 0)
		{
			const LPCSTR errorMessage = errors->GetStringPointer();
			LOG_FATAL("{}", errorMessage);
		}

		// Get shader reflection data.
		Microsoft::WRL::ComPtr<IDxcBlob> reflectionBlob{};
		ThrowIfFailed(compiledShaderBuffer->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionBlob), nullptr));

		const DxcBuffer reflectionBuffer
		{
			.Ptr = reflectionBlob->GetBufferPointer(),
			.Size = reflectionBlob->GetBufferSize(),
			.Encoding = 0,
		};

		Microsoft::WRL::ComPtr<ID3D12ShaderReflection> shaderReflection{};
		utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&shaderReflection));
		D3D12_SHADER_DESC shaderDesc{};
		shaderReflection->GetDesc(&shaderDesc);


		// Setup the input assembler. Only applicable for vertex shaders.
		if (stage == EShaderStage::Vertex)
		{
			for (const uint32_t parameterIndex : std::views::iota(0u, shaderDesc.InputParameters))
			{
				D3D12_SIGNATURE_PARAMETER_DESC signatureParameterDesc{};
				shaderReflection->GetInputParameterDesc(parameterIndex, &signatureParameterDesc);


				ColorFormat format = ColorFormat::UNDEFINED;
				switch (signatureParameterDesc.ComponentType)
				{
				case D3D_REGISTER_COMPONENT_UINT32:
					switch (signatureParameterDesc.Mask)
					{
				case 0b0001:
					format = ColorFormat::R32_UINT;
						break;
					default:
						LOG_FATAL("Unknown vertex input type");
					}

				case D3D_REGISTER_COMPONENT_SINT32:
					break;
				case D3D_REGISTER_COMPONENT_FLOAT32:
					break;
				default:
					LOG_FATAL("Unknown vertex input type")
				}

				LOG_WARNING("Compile vinput {} : {}", uint8_t(signatureParameterDesc.ComponentType), (uint8_t)signatureParameterDesc.Stream);

				

				//vertex_inputs.emplace_back()
			}
		}

		for (const uint32_t i : std::views::iota(0u, shaderDesc.BoundResources))
		{
			D3D12_SHADER_INPUT_BIND_DESC shaderInputBindDesc{};
			ThrowIfFailed(shaderReflection->GetResourceBindingDesc(i, &shaderInputBindDesc));

			if (shaderInputBindDesc.Type == D3D_SIT_CBUFFER)
			{
				ID3D12ShaderReflectionConstantBuffer* shaderReflectionConstantBuffer = shaderReflection->
					GetConstantBufferByIndex(i);
				D3D12_SHADER_BUFFER_DESC constantBufferDesc{};
				shaderReflectionConstantBuffer->GetDesc(&constantBufferDesc);

				//bindings

				const D3D12_ROOT_PARAMETER1 rootParameter
				{
					.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,
					.Descriptor{
						.ShaderRegister = shaderInputBindDesc.BindPoint,
						.RegisterSpace = shaderInputBindDesc.Space,
						.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
					},
				};

				rootParameters.push_back(rootParameter);
			}
			if (shaderInputBindDesc.Type == D3D_SIT_TEXTURE)
			{
				// For now, each individual texture belongs in its own descriptor table. This can cause the root signature to quickly exceed the 64WORD size limit.
				rootParameterIndexMap[stringToWString(shaderInputBindDesc.Name)] = static_cast<uint32_t>(rootParameters.size());
				const CD3DX12_DESCRIPTOR_RANGE1 srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
					1u,
					shaderInputBindDesc.BindPoint,
					shaderInputBindDesc.Space,
					D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

				descriptorRanges.push_back(srvRange);

				const D3D12_ROOT_PARAMETER1 rootParameter
				{
					.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
					.DescriptorTable =
					{
						.NumDescriptorRanges = 1u,
						.pDescriptorRanges = &descriptorRanges.back(),
					},
					.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
				};

				rootParameters.push_back(rootParameter);
			}
		}

		Microsoft::WRL::ComPtr<IDxcBlob> compiledShaderBlob{ nullptr };
		compiledShaderBuffer->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&compiledShaderBlob), nullptr);

		shader.shaderBlob = compiledShaderBlob;
		
exit(0);
	}

	Result<Spirv> ShaderCompiler::load_from_path(const std::filesystem::path& path, const std::string& entry_point,
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
