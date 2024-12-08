#pragma once

#include "gfx/renderer/definition/renderer.hpp"
#include "jobsys/job_sys.hpp"
#include "logger.hpp"
#include "gfx/renderer/definition/render_pass_id.hpp"

#include <functional>
#include <memory>
#include <string>

namespace Eng::Gfx
{
class CustomPassList;
class ImageView;
}

namespace Eng::Gfx
{
class Fence;
class Semaphore;
class Framebuffer;
class Swapchain;
class ImGuiWrapper;
class VkRendererPass;

using SwapchainImageId = uint8_t;
using DeviceImageId    = uint8_t;

class RenderPassInstance
{
public:
    RenderPassInstance(std::weak_ptr<Device> device, const Renderer& renderer, const RenderPassGenericId& rp_ref, bool b_is_present);

    // Should be called before each frame to reset max draw flags
    void         reset_for_next_frame();
    virtual void create_or_resize(const glm::uvec2& viewport, const glm::uvec2& parent, bool b_force = false);
    virtual void render(SwapchainImageId swapchain_image, DeviceImageId device_image);

    // Wait these semaphore before writing to draw targets
    virtual std::vector<const Semaphore*> get_semaphores_to_wait(DeviceImageId device_image) const;

    // Retrieve the fence that will be signaled once the image rendering is finished
    const Fence* get_render_finished_fence(DeviceImageId device_image) const;

    const glm::uvec2& resolution() const
    {
        return current_resolution;
    }

    const glm::uvec2& viewport_resolution() const
    {
        return viewport_res;
    }

    // Get draw pass definition
    const RenderNode& get_definition() const
    {
        return definition;
    }

    // Find the image for the given input attachment generic_name
    std::weak_ptr<ImageView> get_attachment(const std::string& dependency_name) const
    {
        if (attachments_view.empty())
            LOG_FATAL("Attachments have not been initialized yet. Please wait framebuffer update");
        if (auto found = attachments_view.find(dependency_name); found != attachments_view.end())
            return found->second;
        return {};
    }

    std::vector<RenderPassRef> search_dependencies(const RenderPassGenericId& id) const;

    std::weak_ptr<RenderPassInstance> get_dependency(const RenderPassRef& name) const;

    std::weak_ptr<VkRendererPass> get_render_pass_resource() const
    {
        return render_pass_resource;
    }

    std::vector<std::weak_ptr<RenderPassInstance>> all_childs() const;

    void set_resize_callback(const RenderNode::ResizeCallback& in_callback)
    {
        definition.resize_callback(in_callback);
    }

    ImGuiWrapper* imgui() const
    {
        return imgui_context.get();
    }

    std::weak_ptr<Framebuffer> get_current_framebuffer() const
    {
        return framebuffers[current_framebuffer_index];
    }

    std::weak_ptr<CustomPassList> get_custom_passes()
    {
        return custom_passes;
    }

protected:
    bool enable_parallel_rendering() const
    {
        return render_pass_interface && render_pass_interface->record_threads() > 1;
    }

    virtual std::shared_ptr<ImageView> create_view_for_attachment(const std::string& attachment);

    bool                  prepared  = false;
    bool                  submitted = false;
    std::weak_ptr<Device> device;

    // One framebuffer per swapchain or device image
    std::vector<std::shared_ptr<Framebuffer>> framebuffers;

    // One view per attachment
    ankerl::unordered_dense::map<std::string, std::shared_ptr<ImageView>> attachments_view;

    // When we request the recreation of the framebuffers, we need to wait for the next frame to replace it with the new none to be sure
    // we are always submitting valid images
    std::vector<std::shared_ptr<Framebuffer>>                             next_frame_framebuffers;
    ankerl::unordered_dense::map<std::string, std::shared_ptr<ImageView>> next_frame_attachments_view;

    virtual uint8_t get_framebuffer_count() const;

    ankerl::unordered_dense::map<RenderPassRef, std::shared_ptr<RenderPassInstance>> dependencies;

private:
    std::shared_ptr<CustomPassList> custom_passes;
    void                            fill_command_buffer(CommandBuffer& cmd, size_t group_index) const;

    glm::uvec2                    viewport_res{0, 0};
    glm::uvec2                    current_resolution{0, 0};
    RenderNode                    definition;
    std::weak_ptr<VkRendererPass> render_pass_resource;
    std::shared_ptr<IRenderPass>  render_pass_interface;
    std::unique_ptr<ImGuiWrapper> imgui_context;
    uint32_t                      current_framebuffer_index = 0;
};

class TemporaryRenderPassInstance : public RenderPassInstance
{
public:
    static std::shared_ptr<TemporaryRenderPassInstance> create(const std::weak_ptr<Device>& device, const Renderer& renderer);

    bool is_enabled() const
    {
        return enabled;
    }

    void enable(bool in_enabled)
    {
        enabled = in_enabled;
        if (!first_render)
            first_render = true;
    }

    void render(SwapchainImageId swapchain_image, DeviceImageId device_image) override;

private:
    bool first_render = true;
    bool enabled      = true;
    TemporaryRenderPassInstance(const std::weak_ptr<Device>& device, const Renderer& renderer);
};


class CustomPassList
{
public:
    CustomPassList(const std::weak_ptr<Device>& in_device) : device(in_device)
    {
    }

    std::shared_ptr<TemporaryRenderPassInstance> add_custom_pass(const std::vector<RenderPassGenericId>& targets, const Renderer& renderer)
    {
        const auto new_rp = TemporaryRenderPassInstance::create(device, renderer);
        for (const auto& target : targets)
            temporary_dependencies.emplace(target, std::vector<std::shared_ptr<TemporaryRenderPassInstance>>{}).first->second.emplace_back(new_rp);
        return new_rp;
    }

    void remove_custom_pass(const std::shared_ptr<TemporaryRenderPassInstance>& pass)
    {
        for (auto& dep : temporary_dependencies | std::views::values)
        {
            for (int64_t i = dep.size() - 1; i >= 0; --i)
                if (dep[i] == pass)
                    dep.erase(dep.begin() + i);
        }
    }

    std::vector<std::shared_ptr<TemporaryRenderPassInstance>> get_dependencies(const RenderPassGenericId& pass_name) const
    {
        if (auto found = temporary_dependencies.find(pass_name); found != temporary_dependencies.end())
            return found->second;
        return {};
    }

private:
    std::weak_ptr<Device>                                                                                        device;
    ankerl::unordered_dense::map<RenderPassGenericId, std::vector<std::shared_ptr<TemporaryRenderPassInstance>>> temporary_dependencies;
};

} // namespace Eng::Gfx