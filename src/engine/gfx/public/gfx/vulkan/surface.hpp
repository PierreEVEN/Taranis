#pragma once
#include <memory>
#include <string>
#include <vulkan/vulkan_core.h>

namespace Eng::Gfx
{
class CustomPassList;
class SwapchainRenderer;
class Renderer;
class Swapchain;
class Device;
class Window;
class Instance;

class Surface : public std::enable_shared_from_this<Surface>
{
  public:
    static std::shared_ptr<Surface> create(const std::string& name, const std::weak_ptr<Instance>& instance, const std::weak_ptr<Window>& window)
    {
        return std::shared_ptr<Surface>(new Surface(name, instance, window));
    }

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

    void set_device(const std::weak_ptr<Device>& device);
    std::weak_ptr<CustomPassList> set_renderer(const Renderer& renderer);
    void render() const;

    const std::shared_ptr<Swapchain>& get_swapchain() const
    {
        return swapchain;
    }

  private:
    Surface(const std::string& name, const std::weak_ptr<Instance>& instance, const std::weak_ptr<Window>& window);
    std::weak_ptr<Device>      device;
    std::weak_ptr<Window>      window;
    std::weak_ptr<Instance>    instance_ref;
    std::shared_ptr<Swapchain> swapchain;
    std::string                name;
    VkSurfaceKHR               ptr = VK_NULL_HANDLE;
};
} // namespace Eng::Gfx