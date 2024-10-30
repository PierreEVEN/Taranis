#pragma once
#include <memory>
#include <vulkan/vulkan_core.h>

namespace Engine
{
	class Window;
	class Instance;

	class Surface
	{
	public:
		Surface(const std::weak_ptr<Instance>& instance, const std::weak_ptr<Window>& window);
		~Surface();

		VkSurfaceKHR raw() const { return ptr; }

		std::weak_ptr<Window> get_window() const { return window; }

	private:

		std::weak_ptr<Window> window;
		std::weak_ptr<Instance> instance_ref;

		VkSurfaceKHR ptr = VK_NULL_HANDLE;
	};
}
