#include "gfx/vulkan/framebuffer.hpp"

#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/fence.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/semaphore.hpp"
#include "gfx/vulkan/vk_render_pass.hpp"
#include "jobsys/job_sys.hpp"

namespace Eng::Gfx
{
const Fence* Framebuffer::get_render_finished_fence() const
{
    return render_finished_fence.get();
}

Framebuffer::Framebuffer(std::weak_ptr<Device> in_device, const RenderPassInstance& render_pass, size_t image_index, const std::vector<std::shared_ptr<ImageView>>& render_targets) : DeviceResource(std::move(in_device))
{
    auto name                 = render_pass.get_definition().render_pass_ref.to_string();
    render_finished_fence      = Fence::create(name + "_cmd", device(), true);
    command_buffer             = CommandBuffer::create(name + "_cmd", device(), QueueSpecialization::Graphic);
    render_finished_semaphores = Semaphore::create(name + "_sem", device());

    std::vector<VkImageView> views;
    for (const auto& view : render_targets)
        views.emplace_back(view->raw()[image_index]);

    assert(!render_targets.empty());

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

std::shared_ptr<Framebuffer> Framebuffer::create(std::weak_ptr<Device> device, const RenderPassInstance& render_pass, size_t image_index, const std::vector<std::shared_ptr<ImageView>>& render_targets,
                                                 bool require_secondary)
{
    auto fb = std::shared_ptr<Framebuffer>(new Framebuffer(std::move(device), render_pass, image_index, render_targets));
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
    render_finished_fence->wait();
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