#pragma once
#include "gfx/renderer/definition/render_pass_id.hpp"
#include "gfx/renderer/definition/renderer.hpp"
#include "logger.hpp"
#include "gfx/vulkan/device.hpp"

#include <ankerl/unordered_dense.h>
#include <glm/vec2.hpp>
#include <memory>

namespace Eng::Gfx
{
class IRenderPass;
class CommandBuffer;
class CustomPassList;
class Framebuffer;
class ImGuiWrapper;
class VkRendererPass;
class RenderPassInstance;
class ImageView;
class Semaphore;
class Fence;
class Renderer;
class Device;

using SwapchainImageId = uint8_t;
using DeviceImageId    = uint8_t;

class RenderPassInstanceBase : public DeviceResource
{
public:
    RenderPassInstanceBase(std::weak_ptr<Device> device, const Renderer& renderer, const RenderPassGenericId& rp_ref, bool b_is_present);

    // Should be called before each frame to reset max draw flags
    void         reset_for_next_frame();
    virtual void create_or_resize(const glm::uvec2& viewport, const glm::uvec2& parent, bool b_force = false);
    void         render(SwapchainImageId swapchain_image, DeviceImageId device_image);

    /**
     * The resolution of this current pass
     */
    const glm::uvec2& resolution() const
    {
        return current_resolution;
    }

    /**
     * The internal resolution of the plateform window
     */
    const glm::uvec2& viewport_resolution() const
    {
        return viewport_res;
    }

    // Get draw pass definition
    const RenderNode& get_definition() const
    {
        return definition;
    }

    /**
     * Iterate over dependencies
     */
    void for_each_dependency(const std::function<void(const std::shared_ptr<RenderPassInstanceBase>&)>& callback) const
    {
        for (const auto& dependency : dependencies)
            callback(dependency.second);
        for (const auto& dependency : custom_passes->get_dependencies(definition.render_pass_ref.generic_id()))
            callback(dependency);
    }

    /**
     * Iterate over dependencies that match the given filter
     */
    void for_each_dependency(const RenderPassGenericId& id, const std::function<void(const std::shared_ptr<RenderPassInstanceBase>&)>& callback) const
    {
        for (const auto& dependency : dependencies)
            if (dependency.first.generic_id() == id)
                callback(dependency.second);
        for (const auto& dependency : custom_passes->get_dependencies(definition.render_pass_ref.generic_id()))
            if (dependency.generic_id() == id)
                callback(dependency);
    }

    std::weak_ptr<RenderPassInstanceBase> get_dependency(const RenderPassRef& name) const
    {
        if (auto found = dependencies.find(name); found != dependencies.end())
            return found->second;
        for (const auto& temp_child : custom_passes->get_dependencies(definition.render_pass_ref.generic_id()))
            if (temp_child->get_definition().render_pass_ref == name)
                return temp_child;
        return {};
    }

    // Find the image for the given input attachment generic_name
    std::weak_ptr<ImageView> get_attachment(const std::string& attachment_name) const
    {
        if (attachments_view.empty())
            LOG_FATAL("Attachments have not been initialized yet. Please wait framebuffer update");
        if (auto found = attachments_view.find(attachment_name); found != attachments_view.end())
            return found->second;
        return {};
    }

    void set_resize_callback(const RenderNode::ResizeCallback& in_callback)
    {
        definition.resize_callback(in_callback);
    }

    std::weak_ptr<CustomPassList> get_custom_passes()
    {
        return custom_passes;
    }

    std::weak_ptr<Framebuffer> get_current_framebuffer() const
    {
        return framebuffers[current_framebuffer_index];
    }

  protected:
    // Retrieve the fence that will be signaled once the image rendering is finished
    const Fence* get_render_finished_fence(DeviceImageId device_image) const;

    bool enable_parallel_rendering() const
    {
        return render_pass_interface && render_pass_interface->record_threads() > 1;
    }

    virtual std::shared_ptr<ImageView> create_view_for_attachment(const std::string& attachment);
    virtual void                       render_internal(SwapchainImageId swapchain_image, DeviceImageId device_image) = 0;

    std::shared_ptr<IRenderPass> render_pass_interface;

    virtual uint8_t get_framebuffer_count() const;
    virtual void    fill_command_buffer(CommandBuffer& cmd, size_t group_index) const;

private:
    bool prepared  = false;
    bool submitted = false;

    // One framebuffer per swapchain or device image
    std::vector<std::shared_ptr<Framebuffer>> framebuffers;

    // One view per attachment
    ankerl::unordered_dense::map<std::string, std::shared_ptr<ImageView>> attachments_view;

    // When we request the recreation of the framebuffers, we need to wait for the next frame to replace it with the new none to be sure
    // we are always submitting valid images
    std::vector<std::shared_ptr<Framebuffer>>                             next_frame_framebuffers;
    ankerl::unordered_dense::map<std::string, std::shared_ptr<ImageView>> next_frame_attachments_view;

    ankerl::unordered_dense::map<RenderPassRef, std::shared_ptr<RenderPassInstanceBase>> dependencies;
    std::shared_ptr<CustomPassList>                                                      custom_passes;

    glm::uvec2 viewport_res{0, 0};
    glm::uvec2 current_resolution{0, 0};
    RenderNode definition;
    uint32_t   current_framebuffer_index = 0;
};

} // namespace Eng::Gfx