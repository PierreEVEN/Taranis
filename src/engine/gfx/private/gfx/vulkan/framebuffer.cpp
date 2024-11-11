#include "gfx/vulkan/framebuffer.hpp"

#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/semaphore.hpp"
#include "gfx/vulkan/vk_render_pass.hpp"

namespace Eng::Gfx
{
Framebuffer::Framebuffer(std::weak_ptr<Device> in_device, const RenderPassInstance& render_pass, size_t image_index , const std::vector<std::shared_ptr<ImageView>>& render_targets)
    : DeviceResource(std::move(in_device))
{
    auto& name                 = render_pass.get_definition().name;
    command_buffer             = CommandBuffer::create(name + "_cmd", device(), QueueSpecialization::Graphic);
    render_finished_semaphores = Semaphore::create(name + "_sem", device());

    std::vector<VkImageView> views;
    for (const auto& view : render_targets)
        views.emplace_back(view->raw()[image_index]);

    assert(!render_targets.empty());

    VkFramebufferCreateInfo create_infos = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass      = render_pass.get_render_pass_resource().lock()->raw(),
        .attachmentCount = static_cast<uint32_t>(views.size()),
        .pAttachments = views.data(),
        .width = render_pass.resolution().x,
        .height = render_pass.resolution().y,
        .layers = 1,
    };

    VK_CHECK(vkCreateFramebuffer(device().lock()->raw(), &create_infos, nullptr, &ptr), "Failed to create render pass")
    device().lock()->debug_set_object_name(name + "_fb", ptr);
}

Framebuffer::~Framebuffer()
{
    vkDestroyFramebuffer(device().lock()->raw(), ptr, nullptr);
}
} // namespace Eng::Gfx
