#pragma once
#include "gfx/types.hpp"
#include "gfx/renderer/instance/render_pass_instance.hpp"

#include <glm/vec2.hpp>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "gfx/vulkan/swapchain.gen.hpp"

namespace Eng::Gfx
{
class Renderer;
class Fence;
class Semaphore;
class ImageView;
class SwapchainRenderer;
class Surface;
class Device;

class Swapchain final : public RenderPassInstance
{
public:
    REFLECT_BODY()

    static std::shared_ptr<Swapchain> create(const std::weak_ptr<Device>& in_device, const std::weak_ptr<Surface>& surface, Renderer& renderer)
    {
        auto inst = std::shared_ptr<Swapchain>(new Swapchain(in_device, surface, renderer));
        inst->init();
        return inst;
    }

    Swapchain(Swapchain&)  = delete;
    Swapchain(Swapchain&&) = delete;
    virtual ~Swapchain();

    VkSwapchainKHR raw() const
    {
        return ptr;
    }

    static ColorFormat        get_swapchain_format(const std::weak_ptr<Device>& device, const std::weak_ptr<Surface>& surface);
    static VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
    VkPresentModeKHR          choose_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) const;
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

    static Renderer compile_renderer(const std::weak_ptr<Device>& in_device, const std::weak_ptr<Surface>& in_surface, const Renderer& renderer);

protected:
    std::vector<VkSemaphore> get_semaphores_to_wait(DeviceImageId swapchain_image) const override;

    const Fence* get_render_finished_fence(DeviceImageId device_image) const override
    {
        return render_finished_fences[device_image].get();
    }

private:
    Swapchain(const std::weak_ptr<Device>& device, const std::weak_ptr<Surface>& surface, Renderer& renderer);

    std::shared_ptr<ImageView> create_view_for_attachment(const std::string& attachment) override;
    uint8_t                    get_image_count() const override;

    bool render_internal();
    void destroy();

    std::vector<std::shared_ptr<Fence>>     render_finished_fences;
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