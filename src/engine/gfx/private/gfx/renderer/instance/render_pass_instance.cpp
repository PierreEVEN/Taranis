#include "gfx/renderer/instance/render_pass_instance.hpp"

#include "profiler.hpp"
#include "../../../../../job_system/public/jobsys/job_sys.hpp"
#include "gfx/ui/ImGuiWrapper.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/framebuffer.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/semaphore.hpp"
#include "gfx/vulkan/vk_render_pass.hpp"

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

void RenderPassInstance::prepare(SwapchainImageId swapchain_image, DeviceImageId device_image)
{
    // Call reset_for_next_frame() to reset all the draw graph
    if (prepared)
        return;
    prepared                  = true;
    current_framebuffer_index = swapchain_image;
    PROFILER_SCOPE_NAMED(RenderPass_Draw, std::format("Prepare command buffer for render pass {}", definition.name));

    if (framebuffers.empty())
        LOG_FATAL("Framebuffers have not been created using RenderPassInstance::create_or_resize()")
    const auto& framebuffer = framebuffers[current_framebuffer_index];
    auto&       global_cmd  = framebuffer->get_command_buffer();

    if (render_pass_interface && render_pass_interface->record_threads() > 1)
        global_cmd.create_secondaries();
    global_cmd.begin(false);

    global_cmd.begin_debug_marker("BeginRenderPass_" + definition.name, {1, 0, 0, 1});

    for (const auto& child : dependencies)
        child.second->prepare(device_image, device_image);

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
    vkCmdBeginRenderPass(global_cmd.raw(), &begin_infos, global_cmd.get_secondary_command_buffers().empty() ? VK_SUBPASS_CONTENTS_INLINE : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    std::vector<CommandBuffer*> all_cmds;
    if (global_cmd.get_secondary_command_buffers().empty())
    {
        all_cmds = {&global_cmd};
    }
    else
    {
        for (const auto& cmd : global_cmd.get_secondary_command_buffers())
            all_cmds.emplace_back(cmd.second.get());
    }

    {
        if (render_pass_interface)
        {
            if (global_cmd.get_secondary_command_buffers().empty())
            {
                // Set viewport and scissor
                const VkViewport viewport{
                    .x = 0,
                    .y = static_cast<float>(resolution().y),
                    // Flip viewport vertically to avoid textures to being displayed upside down
                    .width = static_cast<float>(resolution().x),
                    .height = -static_cast<float>(resolution().y),
                    .minDepth = 0.0f,
                    .maxDepth = 1.0f,
                };
                vkCmdSetViewport(global_cmd.raw(), 0, 1, &viewport);

                const VkRect2D scissor{
                    .offset = VkOffset2D{0, 0},
                    .extent = VkExtent2D{resolution().x, resolution().y},
                };
                vkCmdSetScissor(global_cmd.raw(), 0, 1, &scissor);

                PROFILER_SCOPE(BuildCommandBufferSync);
                render_pass_interface->render(*this, global_cmd, 0);

                if (imgui_context)
                {
                    PROFILER_SCOPE(RenderPass_DrawUI);
                    imgui_context->prepare_all_window();
                    imgui_context->end(*all_cmds[0]);
                    imgui_context->begin(resolution());
                }
            }
            else
            {

                PROFILER_SCOPE(BuildCommandBufferAsync);
                std::vector<JobHandle<VkCommandBuffer>> handles;
                for (size_t i = 0; i < std::max(1ull, render_pass_interface->record_threads()); ++i)
                {
                    handles.emplace_back(JobSystem::get()
                        .schedule<VkCommandBuffer>(
                            [this, &global_cmd, &all_cmds, i]()
                            {
                                auto found = global_cmd.get_secondary_command_buffers().find(std::this_thread::get_id());
                                if (found == global_cmd.get_secondary_command_buffers().end())
                                    LOG_FATAL("Failed to get secondary command buffer for the current thread");

                                found->second->begin_secondary(*this);

                                // Set viewport and scissor
                                const VkViewport viewport{
                                    .x = 0,
                                    .y = static_cast<float>(resolution().y),
                                    // Flip viewport vertically to avoid textures to being displayed upside down
                                    .width = static_cast<float>(resolution().x),
                                    .height = -static_cast<float>(resolution().y),
                                    .minDepth = 0.0f,
                                    .maxDepth = 1.0f,
                                };
                                vkCmdSetViewport(found->second->raw(), 0, 1, &viewport);

                                const VkRect2D scissor{
                                    .offset = VkOffset2D{0, 0},
                                    .extent = VkExtent2D{resolution().x, resolution().y},
                                };
                                vkCmdSetScissor(found->second->raw(), 0, 1, &scissor);

                                PROFILER_SCOPE(RenderPass_BuildCommandBuffer);
                                render_pass_interface->render(*this, *found->second.get(), i);

                                if (imgui_context && i == 0)
                                {
                                    PROFILER_SCOPE(RenderPass_DrawUI);
                                    imgui_context->prepare_all_window();
                                    imgui_context->end(*all_cmds[0]);
                                    imgui_context->begin(resolution());
                                }
                                found->second->end();
                                return found->second->raw();
                            }));
                }
                std::unordered_set<VkCommandBuffer> cmds_out;
                for (const auto& handle : handles)
                {
                    cmds_out.insert(handle.await());
                }
                std::vector<VkCommandBuffer> cmds_out2(cmds_out.begin(), cmds_out.end());

                vkCmdExecuteCommands(global_cmd.raw(), static_cast<uint32_t>(cmds_out2.size()), cmds_out2.data());
            }
        }
    }

    // End command get
    vkCmdEndRenderPass(framebuffer->get_command_buffer().raw());

    device.lock()->get_instance().lock()->end_debug_marker(framebuffer->get_command_buffer().raw());

    framebuffer->get_command_buffer().end();

}

void RenderPassInstance::submit(SwapchainImageId swapchain_image, DeviceImageId device_image)
{
    if (submitted)
        return;
    submitted = true;

    PROFILER_SCOPE_NAMED(RenderPass_Draw, std::format("Submit command buffer for render pass {}", definition.name));

    for (const auto& child : dependencies)
        child.second->submit(device_image, device_image);

    // Submit get (wait children completion using children_semaphores)
    std::vector<VkSemaphore> children_semaphores;
    for (const auto& semaphore : get_semaphores_to_wait(device_image))
        children_semaphores.emplace_back(semaphore->raw());

    const auto&                       framebuffer = framebuffers[swapchain_image];
    std::vector<VkPipelineStageFlags> wait_stage(children_semaphores.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    const auto                        command_buffer_ptr            = framebuffer->get_command_buffer().raw();
    const auto                        render_finished_semaphore_ptr = framebuffer->render_finished_semaphore().raw();
    const VkSubmitInfo                submit_infos{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = static_cast<uint32_t>(children_semaphores.size()),
        .pWaitSemaphores = children_semaphores.data(),
        .pWaitDstStageMask = wait_stage.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer_ptr,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &render_finished_semaphore_ptr,
    };

    framebuffer->get_command_buffer().submit(submit_infos, get_signal_fence(device_image));
}

std::vector<const Semaphore*> RenderPassInstance::get_semaphores_to_wait(DeviceImageId device_image) const
{
    std::vector<const Semaphore*> semaphores;
    for (const auto& child : dependencies)
        semaphores.emplace_back(&child.second->framebuffers[device_image]->render_finished_semaphore());
    return semaphores;
}

const Fence* RenderPassInstance::get_signal_fence(DeviceImageId) const
{
    return nullptr;
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

void RenderPassInstance::reset_for_next_frame()
{
    prepared  = false;
    submitted = false;
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

void RenderPassInstance::create_or_resize(const glm::uvec2& viewport, const glm::uvec2& parent)
{
    glm::uvec2 desired_resolution = resize_callback ? resize_callback(viewport) : parent;

    for (const auto& dep : dependencies)
        dep.second->create_or_resize(viewport, desired_resolution);

    PROFILER_SCOPE_NAMED(RenderPass_Draw, std::format("Resize render pass {}", definition.name));
    if (desired_resolution == current_resolution && !framebuffers.empty())
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
        next_frame_framebuffers.push_back(Framebuffer::create(device, *this, i, ordered_attachments));

}
} // namespace Eng