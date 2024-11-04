#pragma once
#include "gfx/renderer/definition/render_pass.hpp"

#include <memory>
#include <string>
#include <vulkan/vulkan.h>

namespace Engine
{
class Device;

class VkRendererPass
{
  public:
    VkRendererPass(const std::string& name, const std::weak_ptr<Device>& device, RenderPass::Definition infos);
    VkRendererPass(VkRendererPass&&) = delete;
    VkRendererPass(VkRendererPass&)  = delete;
    ~VkRendererPass();

    VkRenderPass raw() const
    {
        return ptr;
    }

    const RenderPass::Definition& get_infos() const
    {
        return infos;
    }

    std::weak_ptr<Device> get_device() const
    {
        return device;
    }

  private:
    RenderPass::Definition infos;
    std::weak_ptr<Device>  device;
    VkRenderPass           ptr = VK_NULL_HANDLE;
};
} // namespace Engine