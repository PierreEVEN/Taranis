#pragma once
#include "device.hpp"

#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Eng::Gfx
{
class Semaphore;
class RenderPassInstanceBase;
class Device;
class CommandBuffer;

class Framebuffer : public DeviceResource
{
  public:
    static std::shared_ptr<Framebuffer> create(const std::string& name, std::weak_ptr<Device> device, const RenderPassInstanceBase& render_pass, size_t image_index,
                                               const std::vector<std::shared_ptr<ImageView>>& render_targets)
    {
        return std::shared_ptr<Framebuffer>(new Framebuffer(name, std::move(device), render_pass, image_index, render_targets));
    }

    Framebuffer(Framebuffer&&) = delete;
    Framebuffer(Framebuffer&)  = delete;
    ~Framebuffer();

    CommandBuffer& get_command_buffer() const
    {
        return *command_buffer;
    }

    VkFramebuffer raw() const
    {
        return ptr;
    }

    Semaphore& render_finished_semaphore() const
    {
        return *render_finished_semaphores;
    }

  private:
    Framebuffer(const std::string& name, std::weak_ptr<Device> device, const RenderPassInstanceBase& render_pass, size_t image_index, const std::vector<std::shared_ptr<ImageView>>& render_targets);
    std::shared_ptr<Semaphore>     render_finished_semaphores;
    VkFramebuffer                  ptr = VK_NULL_HANDLE;
    std::shared_ptr<CommandBuffer> command_buffer;
};
} // namespace Eng::Gfx
