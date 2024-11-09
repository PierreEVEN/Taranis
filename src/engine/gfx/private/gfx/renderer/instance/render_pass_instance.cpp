#include "gfx/renderer/instance/render_pass_instance.hpp"

#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/framebuffer.hpp"
#include "gfx/vulkan/image.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/semaphore.hpp"
#include "gfx/vulkan/vk_render_pass.hpp"

namespace Eng::Gfx
{

RenderPassInstanceBase::RenderPassInstanceBase(std::string in_name, const std::shared_ptr<VkRendererPass>& in_render_pass, const std::shared_ptr<RenderPassInterface>& in_interface, RenderPass::Definition in_definition)
    : interface(in_interface), device(in_render_pass->get_device()), render_pass(in_render_pass), name(std::move(in_name)), definition(std::move(in_definition))
{
}

void RenderPassInstanceBase::render(uint32_t output_framebuffer, uint32_t current_frame)
{
    // Begin get record
    const auto& framebuffer = framebuffers[output_framebuffer];
    framebuffer->get_command_buffer().begin(false);

    device.lock()->get_instance().lock()->begin_debug_marker(framebuffer->get_command_buffer().raw(), "BeginRenderPass_" + name, {1, 0, 0, 1});

    rendered = true;
    for (const auto& child : children)
        child->render(current_frame, current_frame);

    // Begin render pass
    std::vector<VkClearValue> clear_values;
    for (auto& attachment : definition.attachments)
    {
        VkClearValue clear_value;
        clear_value.color = {{0, 0, 0, 1}};
        if (attachment.is_depth())
            clear_value.depthStencil = {0, 0};
        if (attachment.clear_value().is_color())
            clear_value.color = VkClearColorValue{.float32 = {attachment.clear_value().color().x, attachment.clear_value().color().y, attachment.clear_value().color().z, attachment.clear_value().color().w}};
        else if (attachment.clear_value().is_depth_stencil())
            clear_value.depthStencil = VkClearDepthStencilValue{attachment.clear_value().depth_stencil().x, static_cast<uint32_t>(attachment.clear_value().depth_stencil().y)};
        clear_values.emplace_back(clear_value);
    }

    const VkRenderPassBeginInfo begin_infos = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass.lock()->raw(),
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

    interface->render(*this, framebuffer->get_command_buffer());

    // End command get
    vkCmdEndRenderPass(framebuffer->get_command_buffer().raw());

    device.lock()->get_instance().lock()->end_debug_marker(framebuffer->get_command_buffer().raw());

    framebuffer->get_command_buffer().end();

    // Submit get (wait children completion using children_semaphores)
    std::vector<VkSemaphore> children_semaphores;
    for (const auto& child : children)
        children_semaphores.emplace_back(child->framebuffers[current_frame]->render_finished_semaphore().raw());
    if (const auto& wait_semaphore = get_wait_semaphores(current_frame))
        children_semaphores.emplace_back(wait_semaphore->raw());

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

    framebuffer->get_command_buffer().submit(submit_infos, get_signal_fence(current_frame));
}

void RenderPassInstanceBase::new_frame_internal()
{
    rendered = false;
    for (auto& child : children)
        child->new_frame_internal();
}

RenderPassInstance::RenderPassInstance(const std::string&            in_name, const std::shared_ptr<VkRendererPass>& in_render_pass, const std::shared_ptr<RenderPassInterface>& in_interface,
                                       const RenderPass::Definition& in_definition)
    : RenderPassInstanceBase(in_name, in_render_pass, in_interface, in_definition)
{
    interface->init(device, *this);
}

RenderPassInstance::~RenderPassInstance()
{
    framebuffer_images.clear();
    framebuffer_image_views.clear();
}

void RenderPassInstance::render(uint32_t output_framebuffer, uint32_t current_frame)
{
    if (!replacement_framebuffer_images.empty())
    {
        for (size_t i = 0; i < framebuffers.size(); ++i)
            device.lock()->drop_resource(framebuffers[i], i);

        framebuffer_images      = replacement_framebuffer_images;
        framebuffer_image_views = replacement_framebuffer_image_views;
        framebuffers            = replacement_framebuffers;
        replacement_framebuffer_images.clear();
        replacement_framebuffer_image_views.clear();
        replacement_framebuffers.clear();

    }

    RenderPassInstanceBase::render(output_framebuffer, current_frame);
}

void RenderPassInstance::resize(glm::uvec2 base_resolution)
{
    for (auto& child : children)
        child->resize(base_resolution);

    glm::uvec2 desired_res = resize_callback ? resize_callback(base_resolution) : base_resolution;
    if (desired_res.x == 0)
        desired_res.x = 1;
    if (desired_res.y == 0)
        desired_res.y = 1;

    if (framebuffer_resolution == desired_res)
        return;
    framebuffer_resolution = desired_res;

    for (const auto& attachment : render_pass.lock()->get_infos().attachments)
    {
        const auto image = Image::create(name + "-img_" + attachment.get_name(), device,
                                         ImageParameter{
                                             .format = attachment.get_format(),
                                             .gpu_write_capabilities = ETextureGPUWriteCapabilities::Enabled,
                                             .buffer_type = EBufferType::IMMEDIATE,
                                             .width = framebuffer_resolution.x,
                                             .height = framebuffer_resolution.y,
                                         });
        replacement_framebuffer_images.emplace_back();
        replacement_framebuffer_image_views.emplace_back(ImageView::create(name + "-view_" + attachment.get_name(), image));
    }

    for (size_t i = 0; i < replacement_framebuffer_image_views[0]->raw().size(); ++i)
        replacement_framebuffers.emplace_back(Framebuffer::create(name + "-fb_" + std::to_string(i), render_pass.lock()->get_device(), *this, i, replacement_framebuffer_image_views));

    if (framebuffer_images.empty())
    {
        framebuffer_images      = replacement_framebuffer_images;
        framebuffer_image_views = replacement_framebuffer_image_views;
        framebuffers            = replacement_framebuffers;
        replacement_framebuffer_images.clear();
        replacement_framebuffer_image_views.clear();
        replacement_framebuffers.clear();
    }
}

RendererInstance::RendererInstance(const std::string& in_name, const std::shared_ptr<VkRendererPass>& render_pass_object, const std::shared_ptr<RenderPass>& present_pass)
    : RenderPassInstanceBase(in_name, render_pass_object, present_pass->get_interface(), present_pass->get_infos())
{
    // Instantiate all unique render passes
    std::unordered_map<std::shared_ptr<RenderPass>, std::shared_ptr<RenderPassInstance>> instanced_passes;
    std::vector<std::shared_ptr<RenderPass>>                                             remaining;
    for (const auto& dep : present_pass->get_dependencies())
        remaining.push_back(dep);
    while (!remaining.empty())
    {
        const std::shared_ptr<RenderPass> def = remaining.back();
        remaining.pop_back();
        instanced_passes.emplace(def, std::make_shared<RenderPassInstance>(def->get_name(), render_pass_object->get_device().lock()->find_or_create_render_pass(def->get_infos()), def->get_interface(), def->get_infos()));
        for (const auto& dep : def->get_dependencies())
            remaining.emplace_back(dep);
    }

    // Construct render pass tree
    remaining.push_back(present_pass);
    while (!remaining.empty())
    {
        const std::shared_ptr<RenderPass> def = remaining.back();
        remaining.pop_back();

        for (const auto& dep : def->get_dependencies())
        {
            auto instanced_parent = instanced_passes.find(def);

            if (instanced_parent != instanced_passes.end())
                instanced_parent->second->add_child_render_pass(instanced_passes.find(dep)->second);
            else
                add_child_render_pass(instanced_passes.find(dep)->second);
            remaining.emplace_back(dep);
        }
    }

    interface->init(device, *this);
}

void RendererInstance::render(uint32_t output_framebuffer, uint32_t current_frame)
{
    new_frame_internal();
    RenderPassInstanceBase::render(output_framebuffer, current_frame);
}
} // namespace Eng::Gfx