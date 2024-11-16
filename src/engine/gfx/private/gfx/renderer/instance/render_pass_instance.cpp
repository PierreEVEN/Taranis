#include "gfx/renderer/instance/render_pass_instance.hpp"

#include "profiler.hpp"
#include "gfx/ui/ImGuiWrapper.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/framebuffer.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/semaphore.hpp"
#include "gfx/vulkan/vk_render_pass.hpp"
#include "jobsys/job_sys.hpp"

namespace Eng::Gfx
{
RenderPassInstance::RenderPassInstance(std::weak_ptr<Device> in_device, const Renderer& renderer, const std::string& name, bool b_is_present)
    : device(std::move(in_device)), definition(renderer.get_node(name))
{
    assert(renderer.compiled());

    // Init render pass tree
    for (const auto& dependency : definition.dependencies)
        dependencies.emplace(dependency, std::make_shared<RenderPassInstance>(device, renderer, dependency, false));

    render_pass_resource = device.lock()->find_or_create_render_pass(definition.get_key(b_is_present));

    // Create imgui_context context
    if (definition.b_with_imgui)
    {
        imgui_context = std::make_unique<ImGuiWrapper>(name, render_pass_resource, device, definition.imgui_input_window);
        imgui_context->begin(resolution());
    }

    // Create render pass interface
    if (definition.render_pass_initializer)
    {
        render_pass_interface = definition.render_pass_initializer->construct();
        render_pass_interface->init(*this);
    }
}

void RenderPassInstance::render(SwapchainImageId swapchain_image, DeviceImageId device_image)
{
    // Call reset_for_next_frame() to reset all the draw graph
    if (prepared)
        return;
    prepared                  = true;
    current_framebuffer_index = swapchain_image;
    PROFILER_SCOPE_NAMED(RenderPass_Draw, std::format("Prepare command buffer for render pass {}", definition.name));

    if (framebuffers.empty())
        LOG_FATAL("Framebuffers have not been created using RenderPassInstance::create_or_resize()")
    const auto&    framebuffer = framebuffers[current_framebuffer_index];
    CommandBuffer& global_cmd  = framebuffer->begin();
    global_cmd.begin_debug_marker("BeginRenderPass_" + definition.name, {1, 0, 0, 1});

    for (const auto& child : dependencies | std::views::values)
        child->render(device_image, device_image);

    // Begin draw pass
    std::vector<VkClearValue> clear_values;
    for (auto& attachment : render_pass_resource.lock()->get_key().attachments)
    {
        VkClearValue clear_value;
        clear_value.color = {{0, 0, 0, 1}};
        if (is_depth_format(attachment.color_format))
            clear_value.depthStencil = {0, 0};
        if (attachment.has_clear())
        {
            if (is_depth_format(attachment.color_format))
                clear_value.depthStencil = VkClearDepthStencilValue{attachment.clear_depth_value->x, static_cast<uint32_t>(attachment.clear_depth_value->y)};
            else
                clear_value.color = VkClearColorValue{.float32 = {attachment.clear_color_value->x, attachment.clear_color_value->y, attachment.clear_color_value->z, attachment.clear_color_value->w}};
        }
        clear_values.emplace_back(clear_value);
    }

    const VkRenderPassBeginInfo begin_infos = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass_resource.lock()->raw(),
        .framebuffer = framebuffer->raw(),
        .renderArea =
        {
            .offset = {0, 0},
            .extent = {resolution().x, resolution().y},
        },
        .clearValueCount = static_cast<uint32_t>(clear_values.size()),
        .pClearValues = clear_values.data(),
    };
    vkCmdBeginRenderPass(global_cmd.raw(), &begin_infos, enable_parallel_rendering() ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : VK_SUBPASS_CONTENTS_INLINE);

    if (enable_parallel_rendering())
    {
        PROFILER_SCOPE(BuildCommandBufferAsync);
        std::vector<JobHandle<CommandBuffer*>> handles;
         // Jobs for other threads
        for (size_t i = 0; i < std::max(1ull, render_pass_interface->record_threads()); ++i)
        {
            handles.emplace_back(JobSystem::get().schedule<CommandBuffer*>(
                [this, &framebuffer, i]()
                {
                    auto& cmd = framebuffer->current_cmd();
                    cmd.begin(false);
                    fill_command_buffer(cmd, i);
                    return &cmd;
                }));
        }

        for (const auto& handle : handles)
            (void)handle.await();

        for (const auto& handle : handles)
            handle.await()->end();
    }
    else
    {
        PROFILER_SCOPE(BuildCommandBufferSync);
        fill_command_buffer(global_cmd, 0);
    }

    // End command get
    global_cmd.end_render_pass();
    global_cmd.end_debug_marker();
    global_cmd.end();

    {
        PROFILER_SCOPE_NAMED(RenderPass_Draw, std::format("Submit command buffer for render pass {}", definition.name));

        // Submit get (wait children completion using children_semaphores)
        std::vector<VkSemaphore> children_semaphores;
        for (const auto& semaphore : get_semaphores_to_wait(device_image))
            children_semaphores.emplace_back(semaphore->raw());

        std::vector<VkPipelineStageFlags> wait_stage(children_semaphores.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        const auto                        command_buffer_ptr            = global_cmd.raw();
        const auto                        render_finished_semaphore_ptr = framebuffer->render_finished_semaphore().raw();
        const VkSubmitInfo                submit_infos{
                           .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                           .waitSemaphoreCount   = static_cast<uint32_t>(children_semaphores.size()),
                           .pWaitSemaphores      = children_semaphores.data(),
                           .pWaitDstStageMask    = wait_stage.data(),
                           .commandBufferCount   = 1,
                           .pCommandBuffers      = &command_buffer_ptr,
                           .signalSemaphoreCount = 1,
                           .pSignalSemaphores    = &render_finished_semaphore_ptr,
        };
        global_cmd.submit(submit_infos, framebuffer->get_render_finished_fence());
    }
}

std::vector<const Semaphore*> RenderPassInstance::get_semaphores_to_wait(DeviceImageId device_image) const
{
    std::vector<const Semaphore*> semaphores;
    for (const auto& child : dependencies)
        semaphores.emplace_back(&child.second->framebuffers[device_image]->render_finished_semaphore());
    return semaphores;
}

const Fence* RenderPassInstance::get_render_finished_fence(DeviceImageId device_image) const
{
    return framebuffers[device_image]->get_render_finished_fence();
}

std::weak_ptr<RenderPassInstance> RenderPassInstance::get_dependency(const std::string& name) const
{
    if (auto found = dependencies.find(name); found != dependencies.end())
        return found->second;
    return {};
}

std::shared_ptr<ImageView> RenderPassInstance::create_view_for_attachment(const std::string& attachment_name)
{
    auto attachment = definition.attachments.find(attachment_name);
    if (attachment == definition.attachments.end())
        LOG_FATAL("Attachment {} not found", attachment_name);
    return ImageView::create(attachment_name,
                             Image::create(attachment_name, device,
                                           ImageParameter{
                                               .format = attachment->second.color_format,
                                               .gpu_write_capabilities = ETextureGPUWriteCapabilities::Enabled,
                                               .buffer_type = EBufferType::IMMEDIATE,
                                               .width = resolution().x,
                                               .height = resolution().y,
                                           }));
}

uint8_t RenderPassInstance::get_framebuffer_count() const
{
    return device.lock()->get_image_count();
}

void RenderPassInstance::fill_command_buffer(CommandBuffer& cmd, size_t group_index) const
{
    cmd.set_viewport({
        .y      = static_cast<float>(resolution().y),
        .width  = static_cast<float>(resolution().x),
        .height = -static_cast<float>(resolution().y),
    });
    cmd.set_scissor({0, 0, resolution().x, resolution().y});

    if (render_pass_interface)
    {
        PROFILER_SCOPE(BuildCommandBufferSync);
        render_pass_interface->render(*this, cmd, group_index);
    }

    if (imgui_context && group_index == 0)
    {
        PROFILER_SCOPE(RenderPass_DrawUI);
        imgui_context->prepare_all_window();
        imgui_context->end(cmd);
        imgui_context->begin(resolution());
    }
}

void RenderPassInstance::reset_for_next_frame()
{
    prepared  = false;
    for (const auto& dependency : dependencies | std::views::values)
        dependency->reset_for_next_frame();

    if (!next_frame_framebuffers.empty())
    {
        for (size_t i = 0; i < framebuffers.size(); ++i)
            device.lock()->drop_resource(framebuffers[i], i);
        framebuffers = next_frame_framebuffers;
        next_frame_framebuffers.clear();

        assert(!next_frame_attachments_view.empty());
        attachments_view = next_frame_attachments_view;
        next_frame_attachments_view.clear();

        if (render_pass_interface)
            render_pass_interface->on_create_framebuffer(*this);
    }
}

void RenderPassInstance::create_or_resize(const glm::uvec2& viewport, const glm::uvec2& parent, bool b_force)
{
    glm::uvec2 desired_resolution = resize_callback ? resize_callback(viewport) : parent;

    for (const auto& dep : dependencies)
        dep.second->create_or_resize(viewport, desired_resolution, b_force);

    PROFILER_SCOPE_NAMED(RenderPass_Draw, std::format("Resize render pass {}", definition.name));
    if (!b_force && desired_resolution == current_resolution && !framebuffers.empty())
        return;

    if (desired_resolution.x == 0 || desired_resolution.y == 0)
    {
        LOG_ERROR("Invalid framebuffer resolution : {}x{}", desired_resolution.x, desired_resolution.y);
        if (desired_resolution.x == 0)
            desired_resolution.x = 1;
        if (desired_resolution.y == 0)
            desired_resolution.y = 1;
    }

    current_resolution = desired_resolution;

    next_frame_attachments_view.clear();
    next_frame_framebuffers.clear();

    std::vector<std::shared_ptr<ImageView>> ordered_attachments;
    for (const auto& attachment : render_pass_resource.lock()->get_key().attachments)
    {
        ordered_attachments.push_back(create_view_for_attachment(attachment.name));
        next_frame_attachments_view.emplace(attachment.name, ordered_attachments.back());
    }

    for (uint8_t i = 0; i < get_framebuffer_count(); ++i)
        next_frame_framebuffers.push_back(Framebuffer::create(device, *this, i, ordered_attachments, enable_parallel_rendering()));

}
} // namespace Eng