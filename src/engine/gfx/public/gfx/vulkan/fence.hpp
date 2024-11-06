#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vulkan/vulkan_core.h>

namespace Engine::Gfx
{
class Device;

class Fence
{
  public:
    static std::shared_ptr<Fence> create(const std::string& name, std::weak_ptr<Device> device, bool signaled = false)
    {
        return std::shared_ptr<Fence>(new Fence(name, std::move(device), signaled));
    }

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
    Fence(const std::string& name, std::weak_ptr<Device> device, bool signaled = false);
    VkFence               ptr;
    std::weak_ptr<Device> device;
};
} // namespace Engine
