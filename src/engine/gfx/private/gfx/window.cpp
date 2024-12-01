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
                              [](GLFWwindow* window, int x, int y)
                              {
                                  windows[window]->on_resize.execute(x, y);
                                  windows[window]->render();
                              });

    glfwSetCharCallback(ptr,
                        [](GLFWwindow* window, uint32_t chr)
                        {
                            windows[window]->on_input_char.execute(chr);
                        });

    glfwSetCursorPosCallback(ptr,
                             [](GLFWwindow* window, double x, double y)
                             {
                                 windows[window]->on_change_cursor_pos.execute(x, y);
                             });

    glfwSetMouseButtonCallback(ptr,
                               [](GLFWwindow* window, int button, int action, int mods)
                               {
                                   windows[window]->on_mouse_button.execute(button, action, mods);
                               });

    glfwSetKeyCallback(ptr,
                       [](GLFWwindow* window, int key, int scancode, int action, int mods)
                       {
                           windows[window]->on_input_key.execute(key, scancode, action, mods);
                       });

    glfwSetScrollCallback(ptr,
                          [](GLFWwindow* window, double x, double y)
                          {
                              windows[window]->on_scroll.execute(x, y);
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

std::weak_ptr<CustomPassList> Window::set_renderer(const Renderer& renderer) const
{
    return surface->set_renderer(renderer);
}
} // namespace Eng::Gfx