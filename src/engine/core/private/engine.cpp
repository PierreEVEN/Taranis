#include "engine.hpp"

#include <iostream>
#include <GLFW/glfw3.h>

#include "config.hpp"
#include "gfx/window.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/instance.hpp"
#include "gfx/vulkan/physical_device.hpp"
#include "gfx/vulkan/queue_family.hpp"
#include "gfx/vulkan/surface.hpp"


namespace Engine
{
	Engine::Engine(Config config) : app_config(std::move(config))
	{

		glfwInit();
		gfx_instance = std::make_shared<Instance>(app_config);
	}

	Engine::~Engine()
	{
		windows.clear();
		if (gfx_device)
			gfx_device->destroy_resources();
		gfx_device = nullptr;
		gfx_instance = nullptr;
		glfwTerminate();
	}

	std::weak_ptr<Window> Engine::new_window(const WindowConfig& config)
	{
		const auto window = Window::create(gfx_instance, config);

		if (!gfx_device)
		{
			if (auto physical_device = PhysicalDevice::pick_best_physical_device(
				gfx_instance, app_config, window->get_surface()))
			{
				LOG_INFO("selected physical device {}", physical_device.get().get_device_name());
				gfx_device = Device::create(app_config, *gfx_instance, physical_device.get(), *window->get_surface());
			}
			else
				LOG_FATAL("{}", physical_device.error())
		}
		window->get_surface()->create_swapchain(gfx_device);
		//Todo : OnCreateWindow
		windows.emplace(window->get_id(), window);
		return window;
	}

	void Engine::run()
	{
		while (!windows.empty())
		{
			std::vector<size_t> windows_to_remove;
			for (const auto& [id, window] : windows)
			{
				if (window->render())
					windows_to_remove.emplace_back(id);
			}

			for (const auto& window : windows_to_remove)
				windows.erase(window);
			glfwPollEvents();
			gfx_device->next_frame();
		}
	}
}
