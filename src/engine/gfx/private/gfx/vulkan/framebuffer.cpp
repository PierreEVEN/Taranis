#include "gfx/vulkan/framebuffer.hpp"

#include "gfx/renderer/instance/render_pass_instance.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/semaphore.hpp"
#include "gfx/vulkan/vk_render_pass.hpp"

namespace Engine
{
Framebuffer::Framebuffer(const std::string& name, std::weak_ptr<Device> in_device, const RenderPassInstanceBase& render_pass, size_t image_index) : device(std::move(in_device))
{
    command_buffer = CommandBuffer::create(name + "_cmd", device, QueueSpecialization::Graphic);
    render_finished_semaphores = Semaphore::create(name + "_sem", device);
    const auto rp              = render_pass.get_render_pass().lock();

    std::vector<VkImageView> views;
    for (const auto& view : render_pass.get_attachments())
        views.emplace_back(view.lock()->raw()[image_index]);

    assert(!render_pass.get_attachments().empty());

    VkFramebufferCreateInfo create_infos = {
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass      = rp->raw(),
        .attachmentCount = static_cast<uint32_t>(views.size()),
        .pAttachments    = views.data(),
        .width           = render_pass.resolution().x,
        .height          = render_pass.resolution().y,
        .layers          = 1,
    };

    VK_CHECK(vkCreateFramebuffer(device.lock()->raw(), &create_infos, nullptr, &ptr), "Failed to create render pass")
    device.lock()->debug_set_object_name(name + "_fb", ptr);
}

Framebuffer::~Framebuffer()
{
    vkDestroyFramebuffer(device.lock()->raw(), ptr, nullptr);
}
} // namespace Engine
