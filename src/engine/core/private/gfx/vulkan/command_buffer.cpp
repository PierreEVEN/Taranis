#include <utility>

#include "gfx/vulkan/command_buffer.hpp"

#include "gfx/vulkan/command_pool.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/fence.hpp"

namespace Engine
{
	class Fence;

	CommandBuffer::CommandBuffer(std::weak_ptr<Device> in_device, QueueSpecialization type) : device(std::move(
		                                                                                          in_device)), thread_id(std::this_thread::get_id())
	{
		ptr = device.lock()->get_queues().get_queue(type)->get_command_pool().allocate();
	}

	CommandBuffer::~CommandBuffer()
	{
		device.lock()->get_queues().get_queue(type)->get_command_pool().free(ptr, thread_id);
	}

	void CommandBuffer::begin(bool one_time) const
	{

		const VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = one_time ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : static_cast<VkCommandBufferUsageFlags>(0),
		};

		VK_CHECK(vkBeginCommandBuffer(ptr, &beginInfo), "failed to begin one time command buffer");
	}

	void CommandBuffer::end() const
	{
		vkEndCommandBuffer(ptr);
	}

	void CommandBuffer::submit(const Fence* optional_fence) const
	{
		const VkSubmitInfo submit_infos = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &ptr,
		};
		VK_CHECK(vkQueueSubmit(device.lock()->get_queues().get_queue(type)->raw(), 1, &submit_infos, optional_fence ? optional_fence->raw() : nullptr), "Failed to submit queue")
	}
}
