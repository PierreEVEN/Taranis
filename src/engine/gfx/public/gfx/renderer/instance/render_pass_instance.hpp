#pragma once
#include "gfx/renderer/definition/render_pass.hpp"

#include <glm/vec2.hpp>
#include <memory>
#include <string>
#include <vector>

namespace Engine::Gfx
{
class Framebuffer;
class RenderPassInstance;
class Image;
class RenderPassInterface;
class VkRendererPass;
class ImageView;
class Semaphore;
class Fence;
class Swapchain;

class RenderPassInstanceBase
{
  public:
    RenderPassInstanceBase(std::string name, const std::shared_ptr<VkRendererPass>& render_pass, const std::shared_ptr<RenderPassInterface>& interface, RenderPass::Definition definition);

    [[nodiscard]] const std::weak_ptr<VkRendererPass>& get_render_pass() const
    {
        return render_pass;
    }

    virtual std::vector<std::weak_ptr<ImageView>> get_attachments() const = 0;

    virtual void render(uint32_t output_framebuffer, uint32_t current_frame);

    virtual const Semaphore* get_wait_semaphores(uint32_t) const
    {
        return nullptr;
    }

    virtual const Fence* get_signal_fence(uint32_t) const
    {
        return nullptr;
    }

    void add_child_render_pass(std::shared_ptr<RenderPassInstance> child)
    {
        children.emplace_back(child);
    }

    virtual glm::uvec2 resolution() const = 0;

  protected:
    void                                             new_frame_internal();
    std::shared_ptr<RenderPassInterface>             interface;
    bool                                             rendered = false;
    std::weak_ptr<Device>                            device;
    std::vector<std::shared_ptr<RenderPassInstance>> children;
    std::weak_ptr<VkRendererPass>                    render_pass;
    std::vector<std::shared_ptr<Framebuffer>>        framebuffers;
    std::string                                      name;
    RenderPass::Definition                           definition;
};

class RendererInstance : public RenderPassInstanceBase
{
  public:
    RendererInstance(const std::string& name, const std::shared_ptr<VkRendererPass>& render_pass, const std::shared_ptr<RenderPass>& present_pass);
    void render(uint32_t output_framebuffer, uint32_t current_frame) override;
};

class SwapchainRenderer : public RendererInstance
{
  public:
    SwapchainRenderer(const std::string& name, const std::shared_ptr<VkRendererPass>& render_pass, const std::weak_ptr<Swapchain>& target, const std::shared_ptr<RenderPass>& present_step);
    ~SwapchainRenderer();
    std::vector<std::weak_ptr<ImageView>> get_attachments() const override;

    glm::uvec2 resolution() const override;
    void       resize(glm::uvec2 parent_resolution);

    const Semaphore& get_render_finished_semaphore(uint32_t image_index) const;
    const Semaphore* get_wait_semaphores(uint32_t image_index) const override;
    const Fence*     get_signal_fence(uint32_t image_index) const override;

  private:
    std::weak_ptr<Swapchain> swapchain;
};

class RenderPassInstance : public RenderPassInstanceBase
{
  public:
    RenderPassInstance(const std::string& name, const std::shared_ptr<VkRendererPass>& render_pass, const std::shared_ptr<RenderPassInterface>& interface, const RenderPass::Definition& definition);
    ~RenderPassInstance();

    std::vector<std::weak_ptr<ImageView>> get_attachments() const override
    {
        std::vector<std::weak_ptr<ImageView>> attachments;
        for (const auto& attachment : framebuffer_image_views)
            attachments.emplace_back(attachment);
        return attachments;
    }

    glm::uvec2 resolution() const override
    {
        return framebuffer_resolution;
    }

    void resize(glm::uvec2 base_resolution);

  private:
    glm::uvec2 framebuffer_resolution = {0, 0};

    std::vector<std::shared_ptr<Image>>     framebuffer_images;
    std::vector<std::shared_ptr<ImageView>> framebuffer_image_views;
};

} // namespace Engine::Gfx