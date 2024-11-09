#include "gfx/renderer/instance/swapchain_renderer.hpp"

#include "gfx/ui/ImGuiWrapper.hpp"
#include "gfx/vulkan/framebuffer.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/swapchain.hpp"
#include "gfx/vulkan/vk_render_pass.hpp"

namespace Eng::Gfx
{
SwapchainRenderer::SwapchainRenderer(const std::string& in_name, const std::shared_ptr<VkRendererPass>& in_render_pass, const std::weak_ptr<Swapchain>& in_target, const std::shared_ptr<RenderPass>& present_step)
    : RendererInstance(in_name, in_render_pass, present_step), swapchain(in_target)
{
    resize(swapchain.lock()->get_extent());
    imgui = std::make_shared<ImGuiWrapper>("imgui_renderer", render_pass, device, in_target.lock()->get_surface().lock()->get_window());
    imgui->begin(swapchain.lock()->get_extent());
}

SwapchainRenderer::~SwapchainRenderer()
{
    children.clear();
}

std::vector<std::weak_ptr<ImageView>> SwapchainRenderer::get_attachments() const
{
    return {swapchain.lock()->get_image_view()};
}

glm::uvec2 SwapchainRenderer::resolution() const
{
    return swapchain.lock()->get_extent();
}

void SwapchainRenderer::resize(glm::uvec2 parent_resolution)
{
    for (auto& child : children)
        child->resize(parent_resolution);

    int image_index = 0;
    for (size_t i = 0; i < framebuffers.size(); ++i)
        device.lock()->drop_resource(framebuffers[i], i);
    framebuffers.clear();
    const size_t image_count = swapchain.lock()->get_image_view().lock()->raw().size();
    for (size_t i = 0; i < image_count; ++i)
        framebuffers.emplace_back(Framebuffer::create(name, render_pass.lock()->get_device(), *this, image_index++, {swapchain.lock()->get_image_view().lock()}));
}

const Semaphore& SwapchainRenderer::get_render_finished_semaphore(uint32_t image_index) const
{
    return framebuffers[image_index]->render_finished_semaphore();
}

const Semaphore* SwapchainRenderer::get_wait_semaphores(uint32_t image_index) const
{
    return &swapchain.lock()->get_image_available_semaphore(image_index);
}

const Fence* SwapchainRenderer::get_signal_fence(uint32_t image_index) const
{
    return &swapchain.lock()->get_in_flight_fence(image_index);
}
} // namespace Eng