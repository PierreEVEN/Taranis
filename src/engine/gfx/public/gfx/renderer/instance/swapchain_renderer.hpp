#pragma once
#include "render_pass_instance.hpp"

#include <memory>
#include <string>
#include <vector>
#include <glm/vec2.hpp>

namespace Eng::Gfx
{
class RenderPass;
class VkRendererPass;
class ImageView;
class Fence;
class Semaphore;
class Swapchain;

class SwapchainRenderer : public RendererInstance
{
  public:
    SwapchainRenderer(const std::string& name, const std::shared_ptr<VkRendererPass>& render_pass, const std::weak_ptr<Swapchain>& target, const std::shared_ptr<RenderPass>& present_step);
    ~SwapchainRenderer();
    std::vector<std::weak_ptr<ImageView>> get_attachments() const override;

    glm::uvec2 resolution() const override;
    void       resize(glm::uvec2 parent_resolution) override;

    const Semaphore&      get_render_finished_semaphore(uint32_t image_index) const;
    const Semaphore* get_wait_semaphores(uint32_t image_index) const override;
    const Fence*     get_signal_fence(uint32_t image_index) const override;

  private:
    std::weak_ptr<Swapchain> swapchain;
};

}