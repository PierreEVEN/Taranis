#pragma once
#include <vulkan/vulkan_core.h>

namespace Engine
{
	class Config;

	class Instance
	{
	public:
		Instance(const Config& config);

	private:
		VkInstance ptr = VK_NULL_HANDLE;
	};
}
