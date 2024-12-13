#pragma once

#include "gfx/renderer/definition/renderer.hpp"
#include "render_pass_instance_base.hpp"
#include "gfx/renderer/definition/render_pass_id.hpp"

#include <memory>
#include <string>

namespace Eng::Gfx
{
class CustomPassList;
class ImageView;
}

namespace Eng::Gfx
{
class Fence;
class Semaphore;
class Framebuffer;
class Swapchain;
class ImGuiWrapper;
class VkRendererPass;

class RenderPassInstance : public RenderPassInstanceBase
{
public:
    RenderPassInstance(std::weak_ptr<Device> device, const Renderer& renderer, const RenderPassGenericId& rp_ref, bool b_is_present);


    std::weak_ptr<VkRendererPass> get_render_pass_resource() const
    {
        return render_pass_resource;
    }

    ImGuiWrapper* imgui() const
    {
        return imgui_context.get();
    }

protected:
    void fill_command_buffer(CommandBuffer& cmd, size_t group_index) const override;
    void render_internal(SwapchainImageId swapchain_image, DeviceImageId device_image) override;

private:
    std::weak_ptr<VkRendererPass> render_pass_resource;
    std::unique_ptr<ImGuiWrapper> imgui_context;
};
} // namespace Eng::Gfx