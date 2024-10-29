#include "gfx/window.hpp"

#include <GLFW/glfw3.h>

namespace Engine
{
	Window::Window(const WindowConfig& config)
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		ptr = glfwCreateWindow(config.resolution.x, config.resolution.y, config.name.c_str(), nullptr, nullptr);
		glfwShowWindow(ptr);
	}

	Window::~Window()
	{
		glfwDestroyWindow(ptr);
	}
}
