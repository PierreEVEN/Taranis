#include "gfx/renderer/instance/render_pass_instance.hpp"

#include "gfx/ui/ImGuiWrapper.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/framebuffer.hpp"
#include "gfx/vulkan/semaphore.hpp"
#include "gfx/vulkan/vk_render_pass.hpp"
#include "jobsys/job_sys.hpp"
#include "profiler.hpp"

namespace Eng::Gfx
{
RenderPassInstance::RenderPassInstance(std::weak_ptr<Device> in_device, const Renderer& renderer, const RenderPassGenericId& name, bool b_is_present) : RenderPassInstanceBase(std::move(in_device), renderer, name)
{
    render_pass_resource = device().lock()->declare_render_pass(get_definition().get_key(b_is_present), name);

    // Create imgui_context context
    if (get_definition().b_with_imgui)
    {
        imgui_context = std::make_unique<ImGuiWrapper>(get_definition().render_pass_ref.to_string(), render_pass_resource.lock()->get_name(), device(), get_definition().imgui_input_window);
        imgui_context->begin(resolution());
    }
}

void RenderPassInstance::render_internal(SwapchainImageId swapchain_image, DeviceImageId device_image)
{
    const auto&    framebuffer = get_current_framebuffer().lock();
    CommandBuffer& global_cmd  = framebuffer->begin();
    global_cmd.begin_debug_marker("BeginRenderPass_" + get_definition().render_pass_ref.to_string(), {1, 0, 0, 1});

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
                clear_value.depthStencil =
                    VkClearDepthStencilValue{get_definition().reversed_logarithmic_depth ? attachment.clear_depth_value->x : 1.f - attachment.clear_depth_value->x, static_cast<uint32_t>(attachment.clear_depth_value->y)};
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
    global_cmd.begin_render_pass(get_definition().render_pass_ref, begin_infos, enable_parallel_rendering());

    if (render_pass_interface)
    {
        PROFILER_SCOPE(PreDraw);
        render_pass_interface->pre_draw(*this);
    }

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

    // End command current_thread
    global_cmd.end_render_pass();
    global_cmd.end_debug_marker();
    global_cmd.end();

    if (render_pass_interface)
    {
        PROFILER_SCOPE(PreSubmit);
        render_pass_interface->pre_submit(*this);
    }
    {
        PROFILER_SCOPE_NAMED(RenderPass_Draw, std::format("Submit command buffer for draw pass {}", get_definition().render_pass_ref));

        // Submit current_thread (wait children completion using children_semaphores)
        std::vector<VkSemaphore> children_semaphores = get_semaphores_to_wait();
        std::vector<VkPipelineStageFlags> wait_stage(children_semaphores.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        const auto                        command_buffer_ptr            = global_cmd.raw();
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
        global_cmd.submit(submit_infos, framebuffer->get_render_finished_fence());
    }
}

void RenderPassInstance::fill_command_buffer(CommandBuffer& cmd, size_t group_index) const
{
    RenderPassInstanceBase::fill_command_buffer(cmd, group_index);
    if (imgui_context && group_index == 0)
    {
        PROFILER_SCOPE(RenderPass_DrawUI);
        imgui_context->prepare_all_window(this);
        imgui_context->end(cmd);
        imgui_context->begin(resolution());
    }
}
} // namespace Eng::Gfx