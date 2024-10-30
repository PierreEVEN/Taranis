#include "gfx/window.hpp"

#include <iostream>
#include <GLFW/glfw3.h>

static size_t WINDOW_ID = 0;

namespace Engine
{
	Window::Window(const WindowConfig& config) : id(++WINDOW_ID)
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		ptr = glfwCreateWindow(config.resolution.x, config.resolution.y, config.name.c_str(), nullptr, nullptr);
		glfwShowWindow(ptr);
	}

	Window::~Window()
	{
		glfwDestroyWindow(ptr);
	}

	bool Window::render()
	{
		return glfwWindowShouldClose(ptr) || should_close;
	}
}
