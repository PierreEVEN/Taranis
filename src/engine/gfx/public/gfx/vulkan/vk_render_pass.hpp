#pragma once
#include "gfx/renderer/definition/render_pass.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vulkan/vulkan.h>

namespace Eng::Gfx
{
class Device;

class VkRendererPass
{
  public:
    static std::shared_ptr<VkRendererPass> create(const std::string& name, const std::weak_ptr<Device>& device, RenderPass::Definition infos)
    {
        return std::shared_ptr<VkRendererPass>(new VkRendererPass(name, device, std::move(infos)));
    }

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
    VkRendererPass(const std::string& name, const std::weak_ptr<Device>& device, RenderPass::Definition infos);
    RenderPass::Definition infos;
    std::weak_ptr<Device>  device;
    VkRenderPass           ptr = VK_NULL_HANDLE;
};
} // namespace Eng::Gfx