#pragma once
#include <memory>
#include <string>
#include <vulkan/vulkan_core.h>

namespace Engine
{
class Renderer;
class Swapchain;
class Device;
class Window;
class Instance;

class Surface : public std::enable_shared_from_this<Surface>
{
  public:
    Surface(const std::string& name, const std::weak_ptr<Instance>& instance, const std::weak_ptr<Window>& window);
    Surface(Surface&)  = delete;
    Surface(Surface&&) = delete;
    ~Surface();

    VkSurfaceKHR raw() const
    {
        return ptr;
    }

    std::weak_ptr<Window> get_window() const
    {
        return window;
    }

    void create_swapchain(const std::weak_ptr<Device>& device);
    void set_renderer(const std::shared_ptr<Renderer>& present_pass) const;
    void render() const;

  private:
    std::weak_ptr<Window>      window;
    std::weak_ptr<Instance>    instance_ref;
    std::shared_ptr<Swapchain> swapchain;
    std::string                name;
    VkSurfaceKHR               ptr = VK_NULL_HANDLE;
};
} // namespace Engine
