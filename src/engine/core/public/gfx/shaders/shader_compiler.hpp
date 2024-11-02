#pragma once
#include <string>
#include <vector>

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
	class ShaderCompiler
	{
	public:
		ShaderCompiler();

		Result<Spirv> compile_raw(const std::string& raw, const std::string& entry_point, EShaderStage stage, const std::filesystem::path& path, bool b_debug = false);
		Result<Spirv> load_from_path(const std::filesystem::path& path, const std::string& entry_point, EShaderStage stage, bool b_debug = false);

	private:
		IDxcIncludeHandler* include_handler;
		IDxcCompiler3* compiler = nullptr;
		IDxcUtils* utils = nullptr;
	};
}
