#include <utility>

#include "gfx/vulkan/command_pool.hpp"

#include "profiler.hpp"

#include <ranges>

#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/queue_family.hpp"

namespace Eng::Gfx
{
CommandPool::CommandPool(std::string in_name, std::weak_ptr<Device> in_device, const uint32_t& in_queue_family) : device(std::move(in_device)), queue_family(in_queue_family), name(std::move(in_name))
{
}

PoolLockGuard::PoolLockGuard(CommandPool::Mutex& in_mtx) : mtx(&in_mtx)
{
    lock_thread = std::this_thread::get_id();
    if (mtx->pool_thread_id == lock_thread)
    {
        PROFILER_SCOPE(PoolWaitAvailableLocal);
        mtx->pool_mtx->lock_shared();
    }
    else
    {
        PROFILER_SCOPE(PoolWaitAvailableRemote);
        mtx->pool_mtx->lock();
    }
}

CommandPool::~CommandPool()
{
    for (const auto& val : command_pools | std::views::values)
        vkDestroyCommandPool(device.lock()->raw(), val.pool, nullptr);
}

std::pair<VkCommandBuffer, CommandPool::Mutex> CommandPool::allocate(bool b_secondary, std::thread::id thread_id)
{
    std::lock_guard lock(pool_mutex);
    auto            command_pool = command_pools.find(thread_id);
    if (command_pool == command_pools.end())
    {
        std::stringstream ss;
        ss << thread_id;

        VkCommandPoolCreateInfo create_infos{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, .queueFamilyIndex = queue_family};

        VkCommandPool ptr;
        VK_CHECK(vkCreateCommandPool(device.lock()->raw(), &create_infos, nullptr, &ptr), "Failed to create command pool")
        device.lock()->debug_set_object_name(name + "-$" + ss.str(), ptr);
        command_pools.emplace(thread_id, CommandPoolResource{.pool = ptr, .lock = Mutex(thread_id)});
        command_pool = command_pools.find(thread_id);
    }

    VkCommandBufferAllocateInfo infos{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                      .commandPool = command_pool->second.pool,
                                      .level = b_secondary ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                      .commandBufferCount = 1};

    VkCommandBuffer out;

    PROFILER_SCOPE_NAMED(AllocateCommandBuffer, std::format("Allocate command buffer {}", name));
    PoolLockGuard lk(command_pool->second.lock);
    VK_CHECK(vkAllocateCommandBuffers(device.lock()->raw(), &infos, &out), "failed to allocate command buffer")
    return {out, command_pool->second.lock};
}

void CommandPool::free(VkCommandBuffer command_buffer, std::thread::id thread)
{
    std::lock_guard lock(pool_mutex);
    auto            command_pool = command_pools.find(thread);
    if (command_pool != command_pools.end())
    {
        PROFILER_SCOPE_NAMED(FreeCommandBuffer, std::format("Free command buffer {}", name));
        PoolLockGuard lk(command_pool->second.lock);
        vkFreeCommandBuffers(device.lock()->raw(), command_pool->second.pool, 1, &command_buffer);
    }
}
} // namespace Eng::Gfx