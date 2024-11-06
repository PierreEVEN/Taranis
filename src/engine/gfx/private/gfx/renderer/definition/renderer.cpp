#include "gfx/renderer/definition/renderer.hpp"

#include "gfx/renderer/definition/render_pass.hpp"
#include "gfx/vulkan/swapchain.hpp"

namespace Engine::Gfx
{
std::shared_ptr<RenderPass> Renderer::init_for_swapchain(const Swapchain& swapchain) const
{
    auto renderer_step                = RenderPass::create(pass_name, {Attachment::color("color", swapchain.get_format(), create_infos.clear_color)});
    renderer_step->infos.present_pass = true;
    renderer_step->interface          = interface;
    renderer_step->dependencies       = dependencies;
    return renderer_step;
}
} // namespace Engine