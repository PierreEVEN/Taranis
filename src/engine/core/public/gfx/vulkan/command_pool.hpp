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
		CommandPool(std::weak_ptr<Device> device, const uint32_t& queue_family);
		~CommandPool();
		VkCommandBuffer allocate() const;
		void free(VkCommandBuffer command_buffer) const;
	private:
		std::weak_ptr<Device> device;
		std::mutex pool_mutex;
		VkCommandPool ptr = VK_NULL_HANDLE;
	};
}
