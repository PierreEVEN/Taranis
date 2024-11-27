#include "gfx/window.hpp"

#include <GLFW/glfw3.h>

#include "gfx/vulkan/surface.hpp"

#include <ankerl/unordered_dense.h>

namespace Eng::Gfx
{
static size_t WINDOW_ID = 0;

static ankerl::unordered_dense::map<GLFWwindow*, Window*> windows;

std::shared_ptr<Window> Window::create(const std::weak_ptr<Instance>& instance, const WindowConfig& config)
{
    const auto window = std::shared_ptr<Window>(new Window(config));
    window->surface   = Surface::create(config.name, instance, window);
    return window;
}

void Window::reset_events()
{
    scroll_delta = {0, 0};
}

Window::Window(const WindowConfig& config) : id(++WINDOW_ID)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    ptr = glfwCreateWindow(static_cast<int>(config.resolution.x), static_cast<int>(config.resolution.y), config.name.c_str(), nullptr, nullptr);
    windows.emplace(ptr, this);

    glfwSetWindowSizeCallback(ptr,
                              [](GLFWwindow* window, int, int)
                              {
                                  windows[window]->render();
                              });

    glfwSetScrollCallback(ptr,
                          [](GLFWwindow* window, double x, double y)
                          {
                              windows[window]->scroll_delta = {x, y};
                          });
    glfwShowWindow(ptr);
}

Window::~Window()
{
    glfwDestroyWindow(ptr);
    windows.erase(ptr);
}

bool Window::render() const
{
    auto extent = internal_extent();
    if (extent.x > 0 || extent.y > 0)
        surface->render();
    return glfwWindowShouldClose(ptr) || should_close;
}

glm::uvec2 Window::internal_extent() const
{
    int width, height;
    glfwGetWindowSize(ptr, &width, &height);
    return {width, height};
}

void Window::set_renderer(const Renderer& renderer) const
{
    surface->set_renderer(renderer);
}
} // namespace Eng::Gfx