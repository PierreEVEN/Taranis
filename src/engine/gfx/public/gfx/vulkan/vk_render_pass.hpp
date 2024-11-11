#pragma once
#include "gfx/renderer/definition/renderer.hpp"

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
    static std::shared_ptr<VkRendererPass> create(const std::string& name, const std::weak_ptr<Device>& device, RenderPassKey key)
    {
        return std::shared_ptr<VkRendererPass>(new VkRendererPass(name, device, std::move(key)));
    }

    VkRendererPass(VkRendererPass&&) = delete;
    VkRendererPass(VkRendererPass&)  = delete;
    ~VkRendererPass();

    VkRenderPass raw() const
    {
        return ptr;
    }

    const RenderPassKey& get_key() const
    {
        return key;
    }

    std::weak_ptr<Device> get_device() const
    {
        return device;
    }

  private:
    VkRendererPass(const std::string& name, const std::weak_ptr<Device>& device, RenderPassKey key);
    RenderPassKey          key;
    std::weak_ptr<Device>  device;
    VkRenderPass           ptr = VK_NULL_HANDLE;
};
} // namespace Eng::Gfx