#include "gfx/vulkan/instance.hpp"

#include "config.hpp"

#include <GLFW/glfw3.h>

namespace Engine
{
	Instance::Instance(const Config& config)
	{
		VkApplicationInfo appInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = config.app_name.c_str(),
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "Ashwga",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_API_VERSION_1_3,
		};


		uint32_t glfw_extension_count = 0;
		const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

		VkInstanceCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &appInfo,
			.enabledExtensionCount = glfw_extension_count,
			.ppEnabledExtensionNames = glfw_extensions
		};
		VkResult result = vkCreateInstance(&createInfo, nullptr, &ptr);
	}
}
