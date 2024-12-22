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
#include "jobsys/job_sys.hpp"

namespace Eng::Gfx
{

std::vector<VkSemaphore> RenderPassInstanceBase::get_semaphores_to_wait(DeviceImageId image) const
{
    std::vector<VkSemaphore> children_semaphores;
    for_each_dependency(
        [&](const std::shared_ptr<RenderPassInstanceBase>& dep)
        {
            children_semaphores.emplace_back(dep->render_finished_semaphores[image]->raw());
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

RenderPassInstanceBase::RenderPassInstanceBase(std::weak_ptr<Device> in_device, const Renderer& renderer, const RenderPassGenericId& id) : DeviceResource(id, std::move(in_device)), definition(renderer.get_node(id))
{
    assert(renderer.compiled());
    custom_passes = renderer.get_custom_passes();

    command_buffers = std::make_unique<PassCommandPool>(device(), get_definition().render_pass_ref.generic_id());

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

    PROFILER_SCOPE_NAMED(RenderPass_Draw, std::format("Render pass {}", get_definition().render_pass_ref));
    prepared                = true;
    current_swapchain_image = swapchain_image;

    std::vector<std::pair<RenderPassGenericId, JobHandle<void>>> render_jobs;

    std::vector<std::shared_ptr<RenderPassInstanceBase>> found_dependencies;
    for_each_dependency(
        [&](const std::shared_ptr<RenderPassInstanceBase>& dep)
        {
            found_dependencies.emplace_back(dep);
        });

    if (found_dependencies.size() > 1)
    {
        for (const auto& dep : found_dependencies)
            render_jobs.emplace_back(std::pair{dep->get_definition().render_pass_ref.generic_id(), JobSystem::get().schedule<void>(
                                                   [dep, device_image]
                                                   {
                                                       dep->render(device_image, device_image);
                                                   })});

        for (const auto& task : render_jobs)
        {
            PROFILER_SCOPE_NAMED(RenderPass_Draw, std::format("Wait child pass '{}' finished rendering", task.first));
            task.second.await();
        }
    }
    else
    {
        for (const auto& dep : found_dependencies)
            dep->render(device_image, device_image);
    }

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

std::vector<std::string> RenderPassInstanceBase::get_image_resources() const
{
    std::vector<std::string> image_resources;
    if (!frame_resources)
        return image_resources;
    for (const auto& key : frame_resources->images | std::views::keys)
        image_resources.emplace_back(key);
    return image_resources;
}

VkSemaphore RenderPassInstanceBase::get_render_finished_semaphore() const
{
    return render_finished_semaphores[current_swapchain_image]->raw();
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

PassCommandPool::PassCommandPool(std::weak_ptr<Device> in_device, const std::string& name) : DeviceResource(std::move(name), std::move(in_device))
{
}

CommandBuffer& PassCommandPool::begin_primary(DeviceImageId image)
{
    auto& primary = get_primary(image);
    primary.begin(false);
    return primary;
}

CommandBuffer& PassCommandPool::begin_secondary(DeviceImageId image, const Framebuffer& framebuffer, CommandBuffer& parent)
{
    std::lock_guard lk(lock);

    auto& frame_data = per_frame_data[image];
    if (auto found = frame_data.secondary_command_buffer.find(std::this_thread::get_id()); found != frame_data.secondary_command_buffer.end())
    {
        found->second->set_context(&framebuffer, &parent);
        found->second->begin(false);
        return *found->second;
    }
    auto& found = *frame_data.secondary_command_buffer.emplace(std::this_thread::get_id(), SecondaryCommandBuffer::create(name() + "_primary", device(), QueueSpecialization::Graphic)).first->second;
    found.set_context(&framebuffer, &parent);
    found.begin(false);
    return found;
}

CommandBuffer& PassCommandPool::get_primary(DeviceImageId image)
{
    std::lock_guard lk(lock);
    if (image >= per_frame_data.size())
        per_frame_data.resize(image + 1);

    auto& frame_data = per_frame_data[image];
    if (auto found = frame_data.primary_command_buffers.find(std::this_thread::get_id()); found != frame_data.primary_command_buffers.end())
        return *found->second;
    return *frame_data.primary_command_buffers.emplace(std::this_thread::get_id(), CommandBuffer::create(name() + "_primary", device(), QueueSpecialization::Graphic)).first->second;
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
        [&](const std::shared_ptr<RenderPassInstanceBase>& dep)
        {
            if (!dep->frame_resources)
                dep->create_or_resize(viewport_resolution(), resolution(), false);
            dep->reset_for_next_frame();
        });

    if (next_frame_resources)
    {
        if (frame_resources)
            device().lock()->drop_resource(frame_resources);
        frame_resources      = next_frame_resources;
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
        for (auto& semaphore : render_finished_semaphores)
            semaphore = Semaphore::create(std::format("Render finished semaphore : {}", definition.render_pass_ref), device());
    }

    if (!b_force && desired_resolution == current_resolution && frame_resources)
        return nullptr;

    next_frame_resources = std::make_shared<FrameResources>(device());

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
    const auto           new_rp = RenderPassInstanceBase::create(device, renderer.compile(ColorFormat::UNDEFINED, device));
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