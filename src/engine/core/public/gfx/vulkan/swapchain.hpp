#pragma once
#include <glm/vec2.hpp>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "gfx/renderer/renderer_definition.hpp"

namespace Engine
{
class Fence;
class Semaphore;
class ImageView;
class SwapchainPresentPass;
} // namespace Engine

namespace Engine
{
class Surface;

class Device;

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

class Swapchain : public std::enable_shared_from_this<Swapchain>
{
  public:
    Swapchain(const std::weak_ptr<Device>& device, const std::weak_ptr<Surface>& Surface, bool vsync = false);
    ~Swapchain();

    VkSwapchainKHR raw() const
    {
        return ptr;
    }

    static VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
    static VkPresentModeKHR   choose_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes, bool vsync);
    static glm::uvec2         choose_extent(const VkSurfaceCapabilitiesKHR& capabilities, glm::uvec2 base_extent);

    void create_or_recreate();

    void render();

    std::weak_ptr<Surface> get_surface() const
    {
        return surface;
    }

    void set_renderer(const std::shared_ptr<Renderer>& present_step);

    std::weak_ptr<ImageView> get_image_view() const;

    glm::uvec2 get_extent() const
    {
        return extent;
    }

    ColorFormat get_format() const
    {
        return swapchain_format;
    }

    const Semaphore& get_image_available_semaphore(uint32_t image_index) const;
    const Fence&     get_in_flight_fence(uint32_t image_index) const;

  private:
    bool vsync = true;

    bool render_internal();

    void destroy();

    std::weak_ptr<Device>                   device;
    std::weak_ptr<Surface>                  surface;
    VkSwapchainKHR                          ptr              = VK_NULL_HANDLE;
    ColorFormat                             swapchain_format = ColorFormat::UNDEFINED;
    glm::uvec2                              extent           = {};
    std::vector<VkImage>                    swapChainImages;
    std::shared_ptr<ImageView>              image_view;
    std::shared_ptr<SwapchainPresentPass>   renderer;
    std::vector<std::shared_ptr<Semaphore>> image_available_semaphores;
    std::vector<std::shared_ptr<Fence>>     in_flight_fences;
};
} // namespace Engine
