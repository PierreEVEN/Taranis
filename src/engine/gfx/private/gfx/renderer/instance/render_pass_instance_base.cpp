#include "gfx/renderer/instance/render_pass_instance_base.hpp"

#include "profiler.hpp"
#include "gfx/renderer/instance/compute_pass_instance.hpp"
#include "gfx/renderer/instance/render_pass_instance.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/framebuffer.hpp"
#include "gfx/vulkan/image.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/semaphore.hpp"

namespace Eng::Gfx
{

RenderPassInstanceBase::RenderPassInstanceBase(std::weak_ptr<Device> in_device, const Renderer& renderer, const RenderPassGenericId& name)
    : DeviceResource(std::move(in_device)), definition(renderer.get_node(name))
{
    assert(renderer.compiled());
    custom_passes = renderer.get_custom_passes();

    // Init draw pass tree
    for (const auto& dependency : definition.dependencies)
    {
        const auto& dependency_node = renderer.get_node(dependency);
        dependencies.emplace(dependency_node.render_pass_ref, create(device(), renderer, dependency));
    }

    // Instantiate render pass interface
    if (definition.render_pass_initializer)
    {
        render_pass_interface = definition.render_pass_initializer->construct();
        render_pass_interface->init(*this);
    }
}

void RenderPassInstanceBase::render(SwapchainImageId swapchain_image, DeviceImageId device_image)
{
    // Call reset_for_next_frame() to reset max the draw graph
    if (prepared)
        return;
    prepared                  = true;
    current_framebuffer_index = swapchain_image;
    PROFILER_SCOPE_NAMED(RenderPass_Draw, std::format("Prepare command buffer for draw pass {}", definition.render_pass_ref));

    for_each_dependency(
        [&](const std::shared_ptr<RenderPassInstanceBase>& dep)
        {
            dep->render(device_image, device_image);
        });

    render_internal(swapchain_image, device_image);
}

void RenderPassInstanceBase::for_each_dependency(const std::function<void(const std::shared_ptr<RenderPassInstanceBase>&)>& callback) const
{
    for (const auto& dependency : dependencies)
        callback(dependency.second);
    custom_passes->for_each_dependency(definition.render_pass_ref.generic_id(), callback);
}

void RenderPassInstanceBase::for_each_dependency(const RenderPassGenericId& id, const std::function<void(const std::shared_ptr<RenderPassInstanceBase>&)>& callback) const
{
    for (const auto& dependency : dependencies)
        if (dependency.first.generic_id() == id)
            callback(dependency.second);
    custom_passes->for_each_dependency(definition.render_pass_ref.generic_id(),
                                       [&](const std::shared_ptr<RenderPassInstanceBase>& dep)
                                       {
                                           if (dep->get_definition().render_pass_ref.generic_id() == id)
                                               callback(dep);
                                       });
}

std::weak_ptr<RenderPassInstanceBase> RenderPassInstanceBase::get_dependency(const RenderPassRef& ref) const
{
    if (auto found = dependencies.find(ref); found != dependencies.end())
        return found->second;
    return custom_passes->get_dependency(definition.render_pass_ref.generic_id(), ref);
}

std::weak_ptr<ImageView> RenderPassInstanceBase::get_image_resource(const std::string& resource_name) const
{
    if (frame_resources)
        if (auto found = frame_resources->images.find(resource_name); found != frame_resources->images.end())
            return found->second;
    return {};
}

std::weak_ptr<Buffer> RenderPassInstanceBase::get_buffer_resource(const std::string& resource_name) const
{
    if (frame_resources)
        if (auto found = frame_resources->buffers.find(resource_name); found != frame_resources->buffers.end())
            return found->second;
    return {};
}

const Fence* RenderPassInstanceBase::get_render_finished_fence(DeviceImageId device_image) const
{
    return render_finished_fences[device_image].get();
}

std::shared_ptr<ImageView> RenderPassInstanceBase::create_view_for_attachment(const std::string& attachment_name)
{
    auto attachment = definition.find_attachment_by_name(attachment_name);
    if (!attachment)
        LOG_FATAL("Attachment {} not found", attachment_name);
    return ImageView::create(attachment_name, Image::create(attachment_name, device(),
                                                            ImageParameter{
                                                                .format = attachment->color_format,
                                                                .gpu_write_capabilities = ETextureGPUWriteCapabilities::Enabled,
                                                                .buffer_type = EBufferType::IMMEDIATE,
                                                                .width = resolution().x,
                                                                .height = resolution().y,
                                                            }));
}

uint8_t RenderPassInstanceBase::get_framebuffer_count() const
{
    return device().lock()->get_image_count();
}

void RenderPassInstanceBase::fill_command_buffer(CommandBuffer& cmd, size_t group_index) const
{
    cmd.set_viewport({
        .y = static_cast<float>(resolution().y),
        .width = static_cast<float>(resolution().x),
        .height = -static_cast<float>(resolution().y),
    });
    cmd.set_scissor({0, 0, resolution().x, resolution().y});

    if (render_pass_interface)
    {
        PROFILER_SCOPE(BuildCommandBufferSync);
        render_pass_interface->draw(*this, cmd, group_index);
    }
}

std::shared_ptr<RenderPassInstanceBase> RenderPassInstanceBase::create(std::weak_ptr<Device> device, const Renderer& renderer, const RenderPassGenericId& rp_ref)
{
    auto node = renderer.get_node(rp_ref);
    if (node.b_is_compute_pass)
        return std::dynamic_pointer_cast<RenderPassInstanceBase>(ComputePassInstance::create(std::move(device), renderer, rp_ref));

    return std::dynamic_pointer_cast<RenderPassInstanceBase>(RenderPassInstance::create(std::move(device), renderer, rp_ref, false));
}

void RenderPassInstanceBase::reset_for_next_frame()
{
    prepared = false;

    for_each_dependency(
        [](const std::shared_ptr<RenderPassInstanceBase>& dep)
        {
            dep->reset_for_next_frame();
        });

    if (next_frame_resources)
    {
        frame_resources = std::move(*next_frame_resources);
        next_frame_resources = {};
        if (render_pass_interface)
            render_pass_interface->on_create_framebuffer(*this);
    }
}

FrameResources* RenderPassInstanceBase::create_or_resize(const glm::uvec2& viewport, const glm::uvec2& parent, bool b_force)
{
    glm::uvec2 desired_resolution = definition.resize_callback_ptr ? definition.resize_callback_ptr(viewport) : parent;

    viewport_res = viewport;

    for_each_dependency(
        [&](const std::shared_ptr<RenderPassInstanceBase>& dep)
        {
            dep->create_or_resize(viewport, desired_resolution, b_force);
        });

    PROFILER_SCOPE_NAMED(RenderPass_Draw, std::format("Resize draw pass {}", definition.render_pass_ref));

    if (!b_force && desired_resolution == current_resolution && frame_resources)
        return nullptr;

    next_frame_resources = {FrameResources{}};

    if (desired_resolution.x == 0 || desired_resolution.y == 0)
    {
        LOG_ERROR("Invalid framebuffer resolution for pass '{}' : {}x{}", definition.render_pass_ref, desired_resolution.x, desired_resolution.y);
        return nullptr;
    }

    current_resolution = desired_resolution;

    for (const auto& attachment : get_definition().attachments_sorted)
        next_frame_resources->images.emplace(attachment.name, create_view_for_attachment(attachment.name));
    return &*next_frame_resources;
}

std::shared_ptr<RenderPassInstanceBase> CustomPassList::add_custom_pass(const std::vector<RenderPassGenericId>& targets, const Renderer& renderer)
{
    const auto new_rp = RenderPassInstanceBase::create(device, renderer);
    for (const auto& target : targets)
        temporary_dependencies.emplace(target, std::vector<std::shared_ptr<RenderPassInstanceBase>>{}).first->second.emplace(new_rp->get_definition().render_pass_ref, new_rp);
    return new_rp;
}

void CustomPassList::remove_custom_pass(const RenderPassRef& ref)
{
    for (auto& dependencies : temporary_dependencies | std::views::values)
        dependencies.erase(ref);
}

void CustomPassList::for_each_dependency(const RenderPassGenericId& target_id, const std::function<void(const std::shared_ptr<RenderPassInstanceBase>&)>& callback) const
{
    if (auto found = temporary_dependencies.find(target_id); found != temporary_dependencies.end())
    {
        for (const auto& elem : found->second)
            callback(elem.second);
    }
}

std::weak_ptr<RenderPassInstanceBase> CustomPassList::get_dependency(const RenderPassGenericId& target_id, const RenderPassRef& ref) const
{
    if (auto found = temporary_dependencies.find(target_id); found != temporary_dependencies.end())
        if (auto found2 = found->second.find(ref); found2 != found->second.end())
            return found2->second;
    return {};
}
}