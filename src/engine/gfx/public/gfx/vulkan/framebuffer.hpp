#pragma once
#include "device.hpp"

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Eng::Gfx
{
class SecondaryCommandBuffer;
}

namespace Eng::Gfx
{
class Semaphore;
class Device;
class CommandBuffer;

class Framebuffer : public DeviceResource
{
public:
    static std::shared_ptr<Framebuffer> create(std::weak_ptr<Device> device, const RenderPassInstance& render_pass, size_t image_index, const std::vector<std::shared_ptr<ImageView>>& render_targets,
                                               bool                  require_secondary);

    Framebuffer(Framebuffer&&) = delete;
    Framebuffer(Framebuffer&)  = delete;
    ~Framebuffer() override;

    CommandBuffer& begin() const;

    CommandBuffer& current_cmd() const;

    VkFramebuffer raw() const
    {
        return ptr;
    }

    Semaphore& render_finished_semaphore() const
    {
        return *render_finished_semaphores;
    }

    const std::weak_ptr<VkRendererPass>& get_render_pass_resource() const
    {
        return render_pass_resource;
    }

    const Fence* get_render_finished_fence() const;

  private:
    Framebuffer(std::weak_ptr<Device> device, const RenderPassInstance& render_pass, size_t image_index, const std::vector<std::shared_ptr<ImageView>>& render_targets);
    std::shared_ptr<Semaphore>                                                   render_finished_semaphores;
    VkFramebuffer                                                                ptr = VK_NULL_HANDLE;
    std::shared_ptr<CommandBuffer>                                               command_buffer;
    std::weak_ptr<VkRendererPass>                                                render_pass_resource;
    std::unordered_map<std::thread::id, std::shared_ptr<SecondaryCommandBuffer>> secondary_command_buffers;
    std::shared_ptr<Fence>                                                       render_finished_fence;
};
} // namespace Eng::Gfx