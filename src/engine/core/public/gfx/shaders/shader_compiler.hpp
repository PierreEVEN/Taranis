#pragma once
#include <string>
#include <vector>

#include "gfx/vulkan/pipeline.hpp"
#include "gfx/vulkan/shader_module.hpp"
#include "gfx/vulkan/vk_check.hpp"

struct IDxcCompiler3;
struct IDxcIncludeHandler;
struct IDxcUtils;

namespace std::filesystem
{
	class path;
}

namespace Engine
{
	struct ShaderProperties
	{
		EShaderStage stage = EShaderStage::Vertex;
		std::string entry_point = "main";
		std::vector<ShaderModule::Bindings> bindings;
		std::vector<Pipeline::VertexInput> vertex_inputs;
		uint32_t push_constant_size = 0;
		std::vector<uint32_t> spirv;
	};


	class ShaderCompiler
	{
	public:
		ShaderCompiler();

		Result<ShaderProperties> compile_raw(const std::string& raw, const std::string& entry_point, EShaderStage stage, const std::filesystem::path& path, bool b_debug = false);
		Result<ShaderProperties> load_from_path(const std::filesystem::path& path, const std::string& entry_point, EShaderStage stage, bool b_debug = false);

	private:
		IDxcIncludeHandler* include_handler;
		IDxcCompiler3* compiler = nullptr;
		IDxcUtils* utils = nullptr;
	};
}
