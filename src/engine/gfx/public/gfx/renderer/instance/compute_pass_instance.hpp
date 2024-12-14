#pragma once
#include "render_pass_instance_base.hpp"

namespace Eng::Gfx
{
class ComputePassInstance : public RenderPassInstanceBase
{
public:
    static std::shared_ptr<ComputePassInstance> create(std::weak_ptr<Device> device, const Renderer& renderer, const RenderPassGenericId& rp_ref)
    {
        return std::shared_ptr<ComputePassInstance>(new ComputePassInstance(std::move(device), renderer, rp_ref));
    }

private:
    ComputePassInstance(std::weak_ptr<Device> device, const Renderer& renderer, const RenderPassGenericId& rp_ref);

protected:
    void render_internal(SwapchainImageId swapchain_image, DeviceImageId device_image) override;
};

}