#pragma once
#include <memory>
#include <string>
#include <vulkan/vulkan_core.h>

namespace Engine
{
class Device;

class Semaphore
{
  public:
    Semaphore(const std::string& name, std::weak_ptr<Device> device);
    Semaphore(Semaphore&)  = delete;
    Semaphore(Semaphore&&) = delete;
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
