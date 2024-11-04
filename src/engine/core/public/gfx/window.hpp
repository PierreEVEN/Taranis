#pragma once
#include <glm/vec2.hpp>
#include <memory>
#include <string>

namespace Engine
{
class Renderer;
}

namespace Engine
{
class Device;
class Instance;
class Surface;
} // namespace Engine

struct GLFWwindow;

namespace Engine
{
class RenderPass;

struct WindowConfig
{
    std::string name       = "no name";
    glm::uvec2  resolution = {800, 600};
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

    void set_renderer(const std::shared_ptr<Renderer>& present_pass) const;

    std::shared_ptr<Surface> get_surface() const
    {
        return surface;
    }

  private:
    Window(const WindowConfig& config);
    std::shared_ptr<Surface> surface;
    size_t                   id;
    bool                     should_close = false;
    GLFWwindow*              ptr;
};
} // namespace Engine
