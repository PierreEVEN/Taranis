#include "gfx/renderer/instance/render_pass_instance.hpp"

#include "gfx/ui/ImGuiWrapper.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/framebuffer.hpp"
#include "gfx/vulkan/semaphore.hpp"
#include "gfx/vulkan/vk_render_pass.hpp"

namespace Eng::Gfx
{
RenderPassInstance::RenderPassInstance(std::weak_ptr<Device> in_device, const Renderer& renderer, const std::string& name, bool b_is_present)
    : device(std::move(in_device)), definition(renderer.get_node(name))
{
    // Init render pass tree
    for (const auto& dependency : definition.dependencies)
        dependencies.emplace(dependency, std::make_shared<RenderPassInstance>(device, renderer, name, false));

    render_pass_resource = device.lock()->find_or_create_render_pass(definition.get_key(b_is_present));

    // Create imgui context
    if (definition.b_with_imgui)
    {
        imgui = std::make_unique<ImGuiWrapper>(name, render_pass_resource, device, definition.imgui_input_window);
        imgui->begin(resolution());
    }

    // Create render pass interface
    if (definition.render_pass_initializer)
    {
        render_pass_interface = definition.render_pass_initializer->construct();
        render_pass_interface->init(*this);
    }
}

void RenderPassInstance::draw(SwapchainImageId swapchain_image, DeviceImageId device_image)
{
    // Call reset_for_next_frame() to reset all the draw graph
    if (draw_called)
        return;
    draw_called = true;

    // Begin get record
    const auto& framebuffer = framebuffers[swapchain_image];
    auto&       cmd         = framebuffer->get_command_buffer();
    cmd.begin(false);

    cmd.begin_debug_marker("BeginRenderPass_" + definition.name, {1, 0, 0, 1});

    for (const auto& child : dependencies)
        child.second->draw(device_image, device_image);

    // Begin draw pass
    std::vector<VkClearValue> clear_values;
    for (auto& attachment : definition.attachments)
    {
        VkClearValue clear_value;
        clear_value.color = {{0, 0, 0, 1}};
        if (is_depth_format(attachment.second.color_format))
            clear_value.depthStencil = {0, 0};
        if (attachment.second.has_clear())
        {
            if (is_depth_format(attachment.second.color_format))
                clear_value.depthStencil = VkClearDepthStencilValue{attachment.second.clear_depth_value->x, static_cast<uint32_t>(attachment.second.clear_depth_value->y)};
            else
                clear_value.color =
                    VkClearColorValue{.float32 = {attachment.second.clear_color_value->x, attachment.second.clear_color_value->y, attachment.second.clear_color_value->z, attachment.second.clear_color_value->w}};
        }
        clear_values.emplace_back(clear_value);
    }

    const VkRenderPassBeginInfo begin_infos = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass_resource->raw(),
        .framebuffer = framebuffer->raw(),
        .renderArea =
        {
            .offset = {0, 0},
            .extent = {resolution().x, resolution().y},
        },
        .clearValueCount = static_cast<uint32_t>(clear_values.size()),
        .pClearValues = clear_values.data(),
    };
    vkCmdBeginRenderPass(framebuffer->get_command_buffer().raw(), &begin_infos, VK_SUBPASS_CONTENTS_INLINE);

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
    vkCmdSetViewport(framebuffer->get_command_buffer().raw(), 0, 1, &viewport);

    const VkRect2D scissor{
        .offset = VkOffset2D{0, 0},
        .extent = VkExtent2D{resolution().x, resolution().y},
    };
    vkCmdSetScissor(framebuffer->get_command_buffer().raw(), 0, 1, &scissor);

    if (render_pass_interface)
        render_pass_interface->render(*this, framebuffer->get_command_buffer());

    if (imgui)
    {
        imgui->end(framebuffer->get_command_buffer());
        imgui->begin(resolution());
    }

    // End command get
    vkCmdEndRenderPass(framebuffer->get_command_buffer().raw());

    device.lock()->get_instance().lock()->end_debug_marker(framebuffer->get_command_buffer().raw());

    framebuffer->get_command_buffer().end();

    // Submit get (wait children completion using children_semaphores)
    std::vector<VkSemaphore> children_semaphores;
    for (const auto& semaphore : get_semaphores_to_wait(device_image))
        children_semaphores.emplace_back(semaphore->raw());

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

void RenderPassInstance::reset_for_next_frame()
{
    draw_called = false;
    for (const auto& dep : dependencies)
        dep.second->reset_for_next_frame();
}

void RenderPassInstance::try_resize(const glm::uvec2& new_resolution)
{
    if (new_resolution == current_resolution && !framebuffers.empty())
        return;

    current_resolution = new_resolution;

    next_frame_framebuffers.clear();



    for (const auto& attachment : attachments_view)
    next_frame_attachments_view

    for (size_t i = 0; i < )
    next_frame_framebuffers.emplace(Framebuffer::create())

}
} // namespace Eng