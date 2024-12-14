#include "gfx/renderer/instance/compute_pass_instance.hpp"

namespace Eng::Gfx
{

ComputePassInstance::ComputePassInstance(std::weak_ptr<Device> device, const Renderer& renderer, const RenderPassGenericId& rp_ref) : RenderPassInstanceBase(std::move(device), renderer, rp_ref)
{
}

void ComputePassInstance::render_internal(SwapchainImageId swapchain_image, DeviceImageId device_image)
{
}
}