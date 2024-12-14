#pragma once

#include "render_pass_instance_base.hpp"
#include "gfx/renderer/definition/renderer.hpp"
#include "gfx/renderer/definition/render_pass_id.hpp"

namespace Eng::Gfx
{
class Fence;
class Semaphore;
class Framebuffer;
class Swapchain;
class ImGuiWrapper;
class VkRendererPass;
class CustomPassList;
class ImageView;

class RenderPassInstance : public RenderPassInstanceBase
{
public:
    static std::shared_ptr<RenderPassInstance> create(std::weak_ptr<Device> device, const Renderer& renderer, const RenderPassGenericId& rp_ref, bool b_is_present)
    {
        return std::shared_ptr<RenderPassInstance>(new RenderPassInstance(std::move(device), renderer, rp_ref, b_is_present));
    }

    std::weak_ptr<VkRendererPass> get_render_pass_resource() const
    {
        return render_pass_resource;
    }

    ImGuiWrapper* imgui() const
    {
        return imgui_context.get();
    }
    FrameResources* create_or_resize(const glm::uvec2& viewport, const glm::uvec2& parent, bool b_force) override;

protected:
    RenderPassInstance(std::weak_ptr<Device> device, const Renderer& renderer, const RenderPassGenericId& rp_ref, bool b_is_present);

    void fill_command_buffer(CommandBuffer& cmd, size_t group_index) const override;
    void render_internal(SwapchainImageId swapchain_image, DeviceImageId device_image) override;

    // Retrieve a list of VkSemaphores to wait before submitting
    virtual std::vector<VkSemaphore> get_semaphores_to_wait() const;

    std::weak_ptr<Framebuffer> get_current_framebuffer() const
    {
        return framebuffers[get_current_image_index()];
    }

  private:
    std::weak_ptr<VkRendererPass> render_pass_resource;
    std::unique_ptr<ImGuiWrapper> imgui_context;
};
} // namespace Eng::Gfx