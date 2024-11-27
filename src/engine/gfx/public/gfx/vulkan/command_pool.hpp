#pragma once
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <ankerl/unordered_dense.h>
#include <vulkan/vulkan_core.h>

namespace Eng::Gfx
{
class QueueFamily;
class Device;

class CommandPool
{
  public:
    class Mutex
    {
        friend class PoolLockGuard;

      public:
        Mutex()
        {
        }

        Mutex(std::thread::id thread) : pool_thread_id(thread), pool_mtx(std::make_shared<std::shared_mutex>())
        {
        }

      private:
        std::thread::id                    pool_thread_id;
        std::shared_ptr<std::shared_mutex> pool_mtx;
    };

    static std::shared_ptr<CommandPool> create(std::string name, std::weak_ptr<Device> device, const uint32_t& queue_family)
    {
        return std::shared_ptr<CommandPool>(new CommandPool(std::move(name), std::move(device), queue_family));
    }

    CommandPool(CommandPool&)  = delete;
    CommandPool(CommandPool&&) = delete;
    ~CommandPool();
    std::pair<VkCommandBuffer, Mutex> allocate(bool b_secondary, std::thread::id thread_id);
    void                              free(VkCommandBuffer command_buffer, std::thread::id thread);

  private:
    struct CommandPoolResource
    {
        VkCommandPool pool;
        Mutex         lock;
    };

    CommandPool(std::string name, std::weak_ptr<Device> device, const uint32_t& queue_family);
    std::weak_ptr<Device>                                    device;
    std::mutex                                               pool_mutex;
    uint32_t                                                 queue_family;
    ankerl::unordered_dense::map<std::thread::id, CommandPoolResource> command_pools;
    std::string                                              name;
};

class PoolLockGuard
{
  public:
    PoolLockGuard(CommandPool::Mutex& in_mtx);

    ~PoolLockGuard()
    {
        if (mtx->pool_thread_id == lock_thread)
            mtx->pool_mtx->unlock_shared();
        else
            mtx->pool_mtx->unlock();
    }

  private:
    CommandPool::Mutex* mtx;
    std::thread::id     lock_thread;
};

} // namespace Eng::Gfx