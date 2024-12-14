#include "gfx/renderer/instance/render_pass_instance_base.hpp"

#include "profiler.hpp"
#include "gfx/renderer/instance/compute_pass_instance.hpp"
#include "gfx/renderer/instance/render_pass_instance.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/fence.hpp"
#include "gfx/vulkan/framebuffer.hpp"
#include "gfx/vulkan/image.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/semaphore.hpp"

namespace Eng::Gfx
{

std::vector<VkSemaphore> RenderPassInstanceBase::get_semaphores_to_wait() const
{
    std::vector<VkSemaphore> children_semaphores;
    for_each_dependency(
        [&](const std::shared_ptr<RenderPassInstanceBase>& dep)
        {
            children_semaphores.emplace_back(dep->render_finished_semaphores[get_current_image()]->raw());
        });
    return children_semaphores;
}

void RenderPassInstanceBase::init()
{
    // Instantiate render pass interface
    if (definition.render_pass_initializer && !render_pass_interface)
    {
        render_pass_interface = definition.render_pass_initializer->construct();
        render_pass_interface->init(*this);
    }
}

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
}

void RenderPassInstanceBase::render(SwapchainImageId swapchain_image, DeviceImageId device_image)
{
    // Ensure our pass is not called twice. Use reset_for_next_frame() before each frame
    if (prepared)
        return;
    PROFILER_SCOPE_NAMED(RenderPass_Draw, std::format("Prepare command buffer for draw pass {}", definition.render_pass_ref));

    prepared      = true;
    current_image = device_image;

    // @TODO : parallelize
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

std::vector<std::weak_ptr<RenderPassInstanceBase>> RenderPassInstanceBase::get_dependencies(const RenderPassGenericId& generic_name) const
{
    std::vector<std::weak_ptr<RenderPassInstanceBase>> out_deps;
    for_each_dependency(generic_name,
                        [&](const std::shared_ptr<RenderPassInstanceBase>& sub_dep)
                        {
                            out_deps.emplace_back(get_dependency(sub_dep->get_definition().render_pass_ref).lock());
                        });
    return out_deps;
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

VkSemaphore RenderPassInstanceBase::get_render_finished_semaphore() const
{
    return render_finished_semaphores[current_image]->raw();
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

uint8_t RenderPassInstanceBase::get_image_count() const
{
    return device().lock()->get_image_count();
}

void RenderPassInstanceBase::fill_command_buffer(CommandBuffer& cmd, size_t group_index) const
{
    if (render_pass_interface)
    {
        PROFILER_SCOPE(BuildCommandBufferSync);
        render_pass_interface->draw(*this, cmd, group_index);
    }
}

FrameResources::~FrameResources()
{
    for (const auto& framebuffer : framebuffers)
        framebuffer->device().lock()->drop_resource(framebuffer, framebuffer->get_image_index());
    framebuffers.clear();
}

std::shared_ptr<RenderPassInstanceBase> RenderPassInstanceBase::create(std::weak_ptr<Device> device, const Renderer& renderer, const RenderPassGenericId& rp_ref)
{
    auto node = renderer.get_node(rp_ref);
    if (node.b_is_compute_pass)
    {
        auto inst = std::dynamic_pointer_cast<RenderPassInstanceBase>(ComputePassInstance::create(std::move(device), renderer, rp_ref));
        return inst;
    }

    auto inst = std::dynamic_pointer_cast<RenderPassInstanceBase>(RenderPassInstance::create(std::move(device), renderer, rp_ref, false));
    return inst;
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
        frame_resources      = std::move(*next_frame_resources);
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

    if (render_finished_semaphores.size() != get_image_count())
    {
        render_finished_semaphores.resize(get_image_count());
        render_finished_fences.resize(get_image_count());
        for (auto& semaphore : render_finished_semaphores)
            semaphore = Semaphore::create(std::format("Render finished semaphore : {}", definition.render_pass_ref), device());
        for (auto& fence : render_finished_fences)
            fence = Fence::create(std::format("Render fence semaphore : {}", definition.render_pass_ref), device());
    }

    if (!b_force && desired_resolution == current_resolution && frame_resources)
        return nullptr;

    next_frame_resources = {FrameResources{}};

    if (desired_resolution.x == 0 || desired_resolution.y == 0)
    {
        LOG_ERROR("Invalid framebuffers resolution for pass '{}' : {}x{}", definition.render_pass_ref, desired_resolution.x, desired_resolution.y);
        return nullptr;
    }

    current_resolution = desired_resolution;

    for (const auto& attachment : get_definition().attachments_sorted)
        next_frame_resources->images.emplace(attachment.name, create_view_for_attachment(attachment.name));
    return &*next_frame_resources;
}

std::shared_ptr<RenderPassInstanceBase> CustomPassList::add_custom_pass(const std::vector<RenderPassGenericId>& targets, const Renderer& renderer)
{
    const auto           new_rp = RenderPassInstanceBase::create(device, renderer);
    const RenderPassRef& ref    = new_rp->get_definition().render_pass_ref;
    for (const auto& target : targets)
        temporary_dependencies.emplace(target, ankerl::unordered_dense::map<RenderPassRef, std::shared_ptr<RenderPassInstanceBase>>{}).first->second.emplace(ref, new_rp);
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