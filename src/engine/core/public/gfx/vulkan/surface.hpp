#pragma once
#include <memory>
#include <vulkan/vulkan_core.h>

#include "gfx/renderer/renderer_definition.hpp"

namespace Engine
{
	class Swapchain;
	class Device;
	class Window;
	class Instance;

	class Surface : public std::enable_shared_from_this<Surface>
	{
	public:
		Surface(const std::weak_ptr<Instance>& instance, const std::weak_ptr<Window>& window);
		~Surface();

		VkSurfaceKHR raw() const { return ptr; }

		std::weak_ptr<Window> get_window() const { return window; }

		void create_swapchain(const std::weak_ptr<Device>& device);
		void set_renderer(const std::shared_ptr<RendererStep>& present_pass);

	private:

		std::weak_ptr<Window> window;
		std::weak_ptr<Instance> instance_ref;
		std::shared_ptr<Swapchain> swapchain;
		VkSurfaceKHR ptr = VK_NULL_HANDLE;
	};
}
