#include "gfx/vulkan/framebuffer.hpp"

#include "gfx/renderer/instance/render_pass_instance.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/fence.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/vk_render_pass.hpp"
#include "jobsys/job_sys.hpp"

namespace Eng::Gfx
{
Framebuffer::Framebuffer(std::weak_ptr<Device> in_device, const RenderPassInstance& render_pass, uint32_t in_image_index, const FrameResources& resources)
    : DeviceResource(std::move(in_device)), image_index(in_image_index)
{
    auto name                 = render_pass.get_definition().render_pass_ref.to_string();
    command_buffer             = CommandBuffer::create(name + "_cmd", device(), QueueSpecialization::Graphic);

    std::vector<VkImageView> views;
    for (const auto& attachment : render_pass.get_definition().attachments_sorted)
        views.emplace_back(resources.images.find(attachment.name)->second->raw()[image_index]);

    assert(!views.empty());

    VkFramebufferCreateInfo create_infos = {
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass      = render_pass.get_render_pass_resource().lock()->raw(),
        .attachmentCount = static_cast<uint32_t>(views.size()),
        .pAttachments    = views.data(),
        .width           = render_pass.resolution().x,
        .height          = render_pass.resolution().y,
        .layers          = 1,
    };

    render_pass_resource = render_pass.get_render_pass_resource();

    VK_CHECK(vkCreateFramebuffer(device().lock()->raw(), &create_infos, nullptr, &ptr), "Failed to create render pass")
    device().lock()->debug_set_object_name(name + "_fb", ptr);
}

std::shared_ptr<Framebuffer> Framebuffer::create(std::weak_ptr<Device> device, const RenderPassInstance& render_pass, uint32_t image_index, const FrameResources& resources, bool require_secondary)
{
    auto fb = std::shared_ptr<Framebuffer>(new Framebuffer(std::move(device), render_pass, image_index, resources));
    if (require_secondary)
        for (const auto& worker : JobSystem::get().get_workers())
            fb->secondary_command_buffers.emplace(worker->thread_id(), SecondaryCommandBuffer::create(render_pass.get_definition().render_pass_ref.to_string() + "_sec_cmd", fb->command_buffer, fb, worker->thread_id()));
    return fb;
}

Framebuffer::~Framebuffer()
{
    vkDestroyFramebuffer(device().lock()->raw(), ptr, nullptr);
}

CommandBuffer& Framebuffer::begin() const
{
    command_buffer->begin(false);
    return *command_buffer;
}

CommandBuffer& Framebuffer::current_cmd() const
{
    if (auto found = secondary_command_buffers.find(std::this_thread::get_id()); found != secondary_command_buffers.end())
    {
        return *found->second;
    }
    return *command_buffer;
}
} // namespace Eng::Gfx