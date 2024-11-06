#pragma once
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

namespace Engine
{
class QueueFamily;
class Device;

class CommandPool
{
  public:
    static std::shared_ptr<CommandPool> create(std::string name, std::weak_ptr<Device> device, const uint32_t& queue_family)
    {
        return std::shared_ptr<CommandPool>(new CommandPool(std::move(name), std::move(device), queue_family));
    }

    CommandPool(CommandPool&)  = delete;
    CommandPool(CommandPool&&) = delete;
    ~CommandPool();
    VkCommandBuffer allocate();
    void            free(VkCommandBuffer command_buffer, std::thread::id thread);

  private:
    CommandPool(std::string name, std::weak_ptr<Device> device, const uint32_t& queue_family);
    std::weak_ptr<Device>                              device;
    std::mutex                                         pool_mutex;
    uint32_t                                           queue_family;
    std::unordered_map<std::thread::id, VkCommandPool> command_pools;
    std::string                                        name;
};
} // namespace Engine
