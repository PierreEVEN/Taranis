#pragma once

#include "gfx/renderer/definition/renderer.hpp"
#include "jobsys/job_sys.hpp"
#include "logger.hpp"

#include <functional>
#include <memory>
#include <string>

namespace Eng::Gfx
{
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
    RenderPassInstance(std::weak_ptr<Device> device, const Renderer& renderer, const std::string& name, bool b_is_present);

    // Should be called before each frame to reset all draw flags
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

    // Get draw pass definition
    const RenderNode& get_definition() const
    {
        return definition;
    }

    // Find the image for the given input attachment name
    std::weak_ptr<ImageView> get_attachment(const std::string& dependency_name) const
    {
        if (attachments_view.empty())
            LOG_FATAL("Attachments have not been initialized yet. Please wait framebuffer update");
        if (auto found = attachments_view.find(dependency_name); found != attachments_view.end())
            return found->second;
        return {};
    }

    std::weak_ptr<RenderPassInstance> get_dependency(const std::string& name) const;

    std::weak_ptr<VkRendererPass> get_render_pass_resource() const
    {
        return render_pass_resource;
    }

    using ResizeCallback = std::function<glm::uvec2(glm::uvec2)>;

    void set_resize_callback(const ResizeCallback& in_callback)
    {
        resize_callback = in_callback;
    }

    ImGuiWrapper* imgui() const
    {
        return imgui_context.get();
    }

    std::weak_ptr<Framebuffer> get_current_framebuffer() const
    {
        return framebuffers[current_framebuffer_index];
    }

  protected:
    bool enable_parallel_rendering() const
    {
        return render_pass_interface && render_pass_interface->record_threads() > 1;
    }

    virtual std::shared_ptr<ImageView> create_view_for_attachment(const std::string& attachment);

    // Used to determine the desired framebuffer resolution from the window resolution
    ResizeCallback resize_callback;

    bool                  prepared  = false;
    bool                  submitted = false;
    std::weak_ptr<Device> device;

    // One framebuffer per swapchain or device image
    std::vector<std::shared_ptr<Framebuffer>> framebuffers;

    // One view per attachment
    std::unordered_map<std::string, std::shared_ptr<ImageView>> attachments_view;

    // When we request the recreation of the framebuffers, we need to wait for the next frame to replace it with the new one to be sure
    // we are always submitting valid images
    std::vector<std::shared_ptr<Framebuffer>>                   next_frame_framebuffers;
    std::unordered_map<std::string, std::shared_ptr<ImageView>> next_frame_attachments_view;

    virtual uint8_t get_framebuffer_count() const;

    std::unordered_map<std::string, std::shared_ptr<RenderPassInstance>> dependencies;

  private:
    void fill_command_buffer(CommandBuffer& cmd, size_t group_index) const;

    glm::uvec2                    current_resolution{0, 0};
    RenderNode                    definition;
    std::weak_ptr<VkRendererPass> render_pass_resource;
    std::shared_ptr<IRenderPass>  render_pass_interface;
    std::unique_ptr<ImGuiWrapper> imgui_context;
    uint32_t                      current_framebuffer_index = 0;
};
} // namespace Eng::Gfx