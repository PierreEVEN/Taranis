#include "gfx/window.hpp"

#include <GLFW/glfw3.h>

#include "engine.hpp"
#include "gfx/renderer/renderer.hpp"
#include "gfx/renderer/renderer_definition.hpp"

static size_t WINDOW_ID = 0;

namespace Engine
{
std::shared_ptr<Window> Window::create(const std::weak_ptr<Instance>& instance, const WindowConfig& config)
{
    const auto window = std::shared_ptr<Window>(new Window(config));
    window->surface   = std::make_shared<Surface>(instance, window);
    return window;
}

Window::Window(const WindowConfig& config) : id(++WINDOW_ID)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    ptr = glfwCreateWindow(static_cast<int>(config.resolution.x), static_cast<int>(config.resolution.y), config.name.c_str(), nullptr, nullptr);
    glfwShowWindow(ptr);
}

Window::~Window()
{
    glfwDestroyWindow(ptr);
}

bool Window::render()
{
    surface->render();

    return glfwWindowShouldClose(ptr) || should_close;
}

glm::uvec2 Window::internal_extent() const
{
    int width, height;
    glfwGetWindowSize(ptr, &width, &height);
    return {width, height};
}

void Window::set_renderer(const std::shared_ptr<Renderer>& present_pass) const
{
    surface->set_renderer(present_pass);
}
} // namespace Engine
