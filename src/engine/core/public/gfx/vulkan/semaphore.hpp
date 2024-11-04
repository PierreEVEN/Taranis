#pragma once
#include <memory>
#include <vulkan/vulkan_core.h>

namespace Engine
{
class Device;

class Semaphore
{
  public:
    Semaphore(std::weak_ptr<Device> device);
    ~Semaphore();

    VkSemaphore raw() const
    {
        return ptr;
    }

  private:
    VkSemaphore           ptr;
    std::weak_ptr<Device> device;
};
} // namespace Engine
