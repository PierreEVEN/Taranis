#pragma once
#include <memory>
#include <string>
#include <vulkan/vulkan_core.h>

namespace Engine::Gfx
{
class Device;

class Semaphore
{
  public:
    static std::shared_ptr<Semaphore> create(const std::string& name, std::weak_ptr<Device> device)
    {
        return std::shared_ptr<Semaphore>(new Semaphore(name, std::move(device)));
    }
    Semaphore(Semaphore&)  = delete;
    Semaphore(Semaphore&&) = delete;
    ~Semaphore();

    VkSemaphore raw() const
    {
        return ptr;
    }

  private:
    Semaphore(const std::string& name, std::weak_ptr<Device> device);
    VkSemaphore           ptr;
    std::weak_ptr<Device> device;
};
} // namespace Engine::Gfx
