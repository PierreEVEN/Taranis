#pragma once

#include "render_pass_instance_base.hpp"
#include "gfx/renderer/definition/renderer.hpp"
#include "gfx/renderer/definition/render_pass_id.hpp"
#include "gfx/renderer/instance/render_pass_instance.gen.hpp"

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
    REFLECT_BODY()

    static std::shared_ptr<RenderPassInstance> create(std::weak_ptr<Device> device, Renderer& renderer, const RenderPassGenericId& rp_ref, bool b_is_present)
    {
        auto inst = std::shared_ptr<RenderPassInstance>(new RenderPassInstance(std::move(device), renderer, rp_ref, b_is_present));
        inst->init();
        return inst;
    }

    std::weak_ptr<VkRendererPass> get_render_pass_resource() const
    {
        return render_pass_resource;
    }

    ImGuiWrapper* imgui() const
    {
        return imgui_context.get();
    }

    FrameResources* create_or_resize(const glm::uvec2& viewport, const glm::uvec2& parent, bool b_force = false) override;

protected:
    RenderPassInstance(std::weak_ptr<Device> device, Renderer& renderer, const RenderPassGenericId& rp_ref, bool b_is_present);

    void render_internal(SwapchainImageId swapchain_image, DeviceImageId device_image) override;

    void fill_command_buffer(CommandBuffer& cmd, size_t group_index) const override;

private:
    std::weak_ptr<VkRendererPass> render_pass_resource;
    std::unique_ptr<ImGuiWrapper> imgui_context;
};
} // namespace Eng::Gfx