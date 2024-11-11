#include "gfx/window.hpp"

#include <GLFW/glfw3.h>

#include "gfx/vulkan/surface.hpp"

#include <unordered_map>

namespace Eng::Gfx
{
static size_t WINDOW_ID = 0;

static std::unordered_map<GLFWwindow*, Window*> windows;

std::shared_ptr<Window> Window::create(const std::weak_ptr<Instance>& instance, const WindowConfig& config)
{
    const auto window = std::shared_ptr<Window>(new Window(config));
    window->surface   = Surface::create(config.name, instance, window);
    return window;
}

Window::Window(const WindowConfig& config) : id(++WINDOW_ID)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    ptr = glfwCreateWindow(static_cast<int>(config.resolution.x), static_cast<int>(config.resolution.y), config.name.c_str(), nullptr, nullptr);
    assert(windows.emplace(ptr, this).second);

    glfwSetWindowSizeCallback(ptr,
                              [](GLFWwindow* window, int, int)
                              {
                                  windows[window]->render();
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
