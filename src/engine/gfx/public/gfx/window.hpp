#pragma once
#include <glm/vec2.hpp>
#include <memory>
#include <string>

struct GLFWwindow;

namespace Eng::Gfx
{
class CustomPassList;
class SwapchainRenderer;
class Renderer;
class Device;
class Instance;
class Surface;

struct WindowConfig
{
    std::string name       = "no name";
    glm::uvec2  resolution = {1920, 1080};
};

class Window : public std::enable_shared_from_this<Window>
{
  public:
    static std::shared_ptr<Window> create(const std::weak_ptr<Instance>& instance, const WindowConfig& config);
    Window(Window&&) = delete;
    Window(Window&)  = delete;
    ~Window();

    GLFWwindow* raw() const
    {
        return ptr;
    }

    size_t get_id() const
    {
        return id;
    }

    bool render() const;

    glm::uvec2 internal_extent() const;

    void close()
    {
        should_close = true;
    }

    std::weak_ptr<Gfx::CustomPassList> set_renderer(const Renderer& renderer) const;

    std::shared_ptr<Surface> get_surface() const
    {
        return surface;
    }

    const glm::dvec2& get_scroll_delta() const
    {
        return scroll_delta;
    }

    void reset_events();

  private:
    Window(const WindowConfig& config);

    glm::dvec2 scroll_delta;

    std::shared_ptr<Surface> surface;
    size_t                   id;
    bool                     should_close = false;
    GLFWwindow*              ptr;
};
} // namespace Eng::Gfx
