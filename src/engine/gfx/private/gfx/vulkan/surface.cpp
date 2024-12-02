#include "gfx/vulkan/surface.hpp"

#include <vulkan/vulkan_core.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "gfx/vulkan/instance.hpp"
#include "gfx/vulkan/swapchain.hpp"
#include "gfx/vulkan/vk_check.hpp"
#include "gfx/window.hpp"

namespace Eng::Gfx
{
Surface::Surface(const std::string& in_name, const std::weak_ptr<Instance>& instance, const std::weak_ptr<Window>& in_window)
    : window(in_window), instance_ref(instance), name(in_name){VK_CHECK(glfwCreateWindowSurface(instance_ref.lock()->raw(), window.lock()->raw(), nullptr, &ptr), "Failed to create window surface")}

      Surface::~Surface()
{
    swapchain = nullptr;
    vkDestroySurfaceKHR(instance_ref.lock()->raw(), ptr, nullptr);
}

void Surface::set_device(const std::weak_ptr<Device>& in_device)
{
    device = in_device;
}

std::weak_ptr<CustomPassList> Surface::set_renderer(const Renderer& renderer)
{
    if (!device.lock())
        LOG_FATAL("Surface::set_device have not been called !");
    swapchain = Swapchain::create(device, weak_from_this(), renderer);
    return swapchain->get_custom_passes();
}

void Surface::render() const
{
    if (swapchain)
        swapchain->draw();
}
} // namespace Eng::Gfx