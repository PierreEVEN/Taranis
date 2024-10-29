#include "gfx/vulkan/surface.hpp"

#include <vulkan/vulkan_core.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "gfx/window.hpp"
#include "gfx/vulkan/instance.hpp"
#include "gfx/vulkan/vk_check.hpp"

namespace Engine
{
	Surface::Surface(const std::weak_ptr<Instance>& instance, const Window& window): instance_ref(instance)
	{
		VK_CHECK(glfwCreateWindowSurface(instance.lock()->raw(), window.raw(), nullptr, &ptr),
		         "Failed to create window surface");
	}

	Surface::~Surface()
	{
		vkDestroySurfaceKHR(instance_ref.lock()->raw(), ptr, nullptr);
	}
}
