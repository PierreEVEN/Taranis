#pragma once
#include <memory>
#include <string>
#include <vulkan/vulkan_core.h>

namespace Engine
{
class Device;

class Fence
{
  public:
    Fence(const std::string& name, std::weak_ptr<Device> device, bool signaled = false);
    Fence(Fence&)  = delete;
    Fence(Fence&&) = delete;
    ~Fence();

    VkFence raw() const
    {
        return ptr;
    }

    void reset() const;
    void wait() const;

  private:
    VkFence               ptr;
    std::weak_ptr<Device> device;
};
} // namespace Engine
