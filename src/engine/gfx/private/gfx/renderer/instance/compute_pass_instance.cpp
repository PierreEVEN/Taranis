#include "gfx/renderer/instance/compute_pass_instance.hpp"

#include "profiler.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/image_view.hpp"

namespace Eng::Gfx
{

ComputePassInstance::ComputePassInstance(std::weak_ptr<Device> device, Renderer& renderer, const RenderPassGenericId& rp_ref) : RenderPassInstanceBase(std::move(device), renderer, rp_ref)
{
}

void ComputePassInstance::render_internal(SwapchainImageId, DeviceImageId device_image)
{
    CommandBuffer& global_cmd = command_buffers->begin_primary(device_image);
    global_cmd.begin_debug_marker("BeginComputePass_" + get_definition().render_pass_ref.to_string(), {1, 0, 0, 1});

    
    if (render_pass_interface)
    {
        PROFILER_SCOPE(PreDraw);
        render_pass_interface->pre_draw(*this);
    }

    fill_command_buffer(global_cmd, 0);


    for (const auto& attachment : get_image_resources())
    {
        auto resource = get_image_resource(attachment).lock();
        assert(resource->get_base_image());
        resource->get_base_image()->get_resource()->set_image_layout(global_cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    global_cmd.end_debug_marker();
    global_cmd.end();

    if (render_pass_interface)
    {
        PROFILER_SCOPE(PreSubmit);
        render_pass_interface->pre_submit(*this);
    }

    {
        PROFILER_SCOPE_NAMED(RenderPass_Draw, std::format("Submit command buffer for compute pass {}", get_definition().render_pass_ref));
        // Submit current_thread (wait children completion using children_semaphores)
        std::vector<VkSemaphore>          children_semaphores = get_semaphores_to_wait(device_image);
        std::vector<VkPipelineStageFlags> wait_stage(children_semaphores.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        const auto                        command_buffer_ptr            = global_cmd.raw();
        const auto                        render_finished_semaphore_ptr = get_render_finished_semaphore();
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
        global_cmd.submit(submit_infos, get_render_finished_fence(device_image));
    }
}
}