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

namespace Engine
{
Surface::Surface(const std::weak_ptr<Instance>& instance, const std::weak_ptr<Window>& in_window) : window(in_window), instance_ref(instance)
{
    VK_CHECK(glfwCreateWindowSurface(instance_ref.lock()->raw(), window.lock()->raw(), nullptr, &ptr), "Failed to create window surface");
}

Surface::~Surface()
{
    swapchain = nullptr;
    vkDestroySurfaceKHR(instance_ref.lock()->raw(), ptr, nullptr);
}

void Surface::create_swapchain(const std::weak_ptr<Device>& device)
{
    swapchain = std::make_shared<Swapchain>(device, weak_from_this());
}

void Surface::set_renderer(const std::shared_ptr<Renderer>& present_pass)
{
    swapchain->set_renderer(present_pass);
}

void Surface::render()
{
    if (swapchain)
        swapchain->render();
}
} // namespace Engine
