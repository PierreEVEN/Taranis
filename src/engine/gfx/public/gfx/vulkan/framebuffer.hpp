#pragma once

#include "device_resource.hpp"

#include <memory>
#include <ankerl/unordered_dense.h>
#include <vulkan/vulkan_core.h>

namespace Eng::Gfx
{
class FrameResources;
class Fence;
class VkRendererPass;
class Semaphore;
class Device;
class CommandBuffer;
class ImageView;
class RenderPassInstance;
class SecondaryCommandBuffer;

class Framebuffer : public DeviceResource
{
  public:
    static std::shared_ptr<Framebuffer> create(std::weak_ptr<Device> device, const RenderPassInstance& render_pass, uint32_t image_index, const FrameResources& resources);

    Framebuffer(Framebuffer&&) = delete;
    Framebuffer(Framebuffer&)  = delete;
    ~Framebuffer() override;

    VkFramebuffer raw() const
    {
        return ptr;
    }

    const std::weak_ptr<VkRendererPass>& get_render_pass_resource() const
    {
        return render_pass_resource;
    }

    uint32_t get_image_index() const
    {
        return image_index;
    }

  private:
    Framebuffer(std::weak_ptr<Device> device, const RenderPassInstance& render_pass, uint32_t image_index, const FrameResources& resources);
    uint32_t image_index = 0;
    VkFramebuffer                                                                          ptr = VK_NULL_HANDLE;
    std::weak_ptr<VkRendererPass>                                                          render_pass_resource;
};
} // namespace Eng::Gfx