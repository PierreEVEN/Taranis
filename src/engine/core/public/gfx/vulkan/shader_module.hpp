#pragma once
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "gfx/shaders/shader_compiler.hpp"

namespace Engine
{
	class Device;

	class ShaderModule
	{
	public:
		ShaderModule(const std::weak_ptr<Device>& device, ShaderProperties properties);
		~ShaderModule();

		VkShaderModule raw() const { return ptr; }
		const ShaderProperties& infos() const { return properties; }

	private:
		ShaderProperties properties;
		VkShaderModule ptr = VK_NULL_HANDLE;
		std::weak_ptr<Device> device;
	};
}
