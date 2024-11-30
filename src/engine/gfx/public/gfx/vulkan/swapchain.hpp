#pragma once
#include "gfx/renderer/instance/render_pass_instance.hpp"
#include "gfx/types.hpp"

#include <glm/vec2.hpp>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Eng::Gfx
{
class Renderer;
class Fence;
class Semaphore;
class ImageView;
class SwapchainRenderer;
class Surface;
class Device;

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

class Swapchain final : public RenderPassInstance
{
  public:
    static std::shared_ptr<Swapchain> create(const std::weak_ptr<Device>& in_device, const std::weak_ptr<Surface>& surface, const Renderer& renderer, bool vsync = false)
    {
        return std::shared_ptr<Swapchain>(new Swapchain(in_device, surface, renderer, vsync));
    }

    Swapchain(Swapchain&)  = delete;
    Swapchain(Swapchain&&) = delete;
    virtual ~Swapchain();

    VkSwapchainKHR raw() const
    {
        return ptr;
    }

    std::vector<const Semaphore*> get_semaphores_to_wait(SwapchainImageId swapchain_image) const override;

    static ColorFormat        get_swapchain_format(const std::weak_ptr<Device>& device, const std::weak_ptr<Surface>& surface);
    static VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
    static VkPresentModeKHR   choose_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes, bool vsync);
    static glm::uvec2         choose_extent(const VkSurfaceCapabilitiesKHR& capabilities, glm::uvec2 base_extent);

    void create_or_recreate();

    void draw();

    std::weak_ptr<Surface> get_surface() const
    {
        return surface;
    }

    std::weak_ptr<ImageView> get_image_view() const;

    glm::uvec2 get_extent() const
    {
        return extent;
    }

    ColorFormat get_format() const
    {
        return swapchain_format;
    }

    std::weak_ptr<Device> get_device() const
    {
        return device;
    }

  private:
    Swapchain(const std::weak_ptr<Device>& device, const std::weak_ptr<Surface>& surface, const Renderer& renderer, bool vsync);
    bool vsync = true;

    std::shared_ptr<ImageView> create_view_for_attachment(const std::string& attachment) override;
    uint8_t                    get_framebuffer_count() const override;

    bool render_internal();
    void destroy();

    std::weak_ptr<Surface>                  surface;
    VkSwapchainKHR                          ptr              = VK_NULL_HANDLE;
    ColorFormat                             swapchain_format = ColorFormat::UNDEFINED;
    glm::uvec2                              extent           = {};
    std::vector<VkImage>                    swapChainImages;
    std::shared_ptr<ImageView>              image_view;
    std::vector<std::shared_ptr<Semaphore>> image_available_semaphores;
    std::vector<const Fence*>               in_flight_fences;
};
} // namespace Eng::Gfx
