#pragma once
#include <string>
#include <vector>

#include "gfx/vulkan/shader_module.hpp"

struct IDxcUtils;
struct IDxcCompiler;

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

		Spirv compile_raw(const std::string& raw, const std::string& entry_point, EShaderStage stage);
		Spirv load_from_path(const std::filesystem::path& path, const std::string& entry_point, EShaderStage stage);

	private:
		IDxcCompiler* compiler = nullptr;
		IDxcUtils* utils = nullptr;
	};
}
