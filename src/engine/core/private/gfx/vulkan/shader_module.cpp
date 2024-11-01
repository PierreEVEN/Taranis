#include <utility>

#include "gfx/vulkan/shader_module.hpp"

#include "gfx/vulkan/device.hpp"

namespace Engine
{
	ShaderModule::ShaderModule(std::weak_ptr<Device> in_device, const Spirv& spirv, const std::vector<Bindings>& in_bindings, const CreateInfos& in_create_infos) : bindings(in_bindings), create_infos(in_create_infos), device(std::move(in_device))
	{
		const VkShaderModuleCreateInfo vertex_create_infos{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.codeSize = spirv.size() * sizeof(uint32_t),
			.pCode = spirv.data(),
		};
		VK_CHECK(vkCreateShaderModule(device.lock()->raw(), &vertex_create_infos, nullptr, &ptr), "failed to create vertex shader module");
	}

	ShaderModule::~ShaderModule()
	{
		vkDestroyShaderModule(device.lock()->raw(), ptr, nullptr);
	}
}
