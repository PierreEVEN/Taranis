#include <utility>

#include "gfx/vulkan/command_pool.hpp"

#include <ranges>

#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/queue_family.hpp"

namespace Engine
{
	CommandPool::CommandPool(std::weak_ptr<Device> in_device, const uint32_t& in_queue_family) : device(
			std::move(in_device)), queue_family(in_queue_family)
	{
	}

	CommandPool::~CommandPool()
	{
		for (const auto& val : command_pools | std::views::values)
			vkDestroyCommandPool(device.lock()->raw(), val, nullptr);
	}

	VkCommandBuffer CommandPool::allocate()
	{
		std::lock_guard lock(pool_mutex);
		auto command_pool = command_pools.find(std::this_thread::get_id());
		if (command_pool == command_pools.end())
		{
			VkCommandPoolCreateInfo create_infos{
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
				.queueFamilyIndex = queue_family
			};
			VkCommandPool ptr;
			VK_CHECK(vkCreateCommandPool(device.lock()->raw(), &create_infos, nullptr, &ptr),
			         "Failed to create command pool")
			command_pools.emplace(std::this_thread::get_id(), ptr);
			command_pool = command_pools.find(std::this_thread::get_id());
		}


		VkCommandBufferAllocateInfo infos{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = command_pool->second,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};

		VkCommandBuffer out;
		VK_CHECK(vkAllocateCommandBuffers(device.lock()->raw(), &infos, &out), "failed to allocate command buffer");
		return out;
	}

	void CommandPool::free(VkCommandBuffer command_buffer, std::thread::id thread)
	{
		std::lock_guard lock(pool_mutex);
		auto command_pool = command_pools.find(thread);
		if (command_pool != command_pools.end())
		{
			vkFreeCommandBuffers(device.lock()->raw(), command_pool->second, 1, &command_buffer);
		}
	}
}
