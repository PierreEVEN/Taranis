#include "gfx/vulkan/semaphore.hpp"

#include "gfx/vulkan/device.hpp"

namespace Engine
{
Semaphore::Semaphore(std::weak_ptr<Device> in_device) : device(std::move(in_device))
{
    VkSemaphoreCreateInfo create_infos{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(device.lock()->raw(), &create_infos, nullptr, &ptr);
}

Semaphore::~Semaphore()
{
    vkDestroySemaphore(device.lock()->raw(), ptr, nullptr);
}
} // namespace Engine
