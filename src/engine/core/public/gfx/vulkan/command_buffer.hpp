#pragma once
#include <memory>

#include "queue_family.hpp"

namespace Engine
{
	class Fence;
	class Device;

	class CommandBuffer
	{
	public:
		CommandBuffer(std::weak_ptr<Device> device, QueueSpecialization type);
		~CommandBuffer();
		VkCommandBuffer raw() const { return ptr; }

		void begin(bool one_time) const;
		void end() const;
		void submit(VkSubmitInfo submit_infos, const Fence* optional_fence = nullptr) const;

	private:
		VkCommandBuffer ptr;
		QueueSpecialization type;
		std::weak_ptr<Device> device;
		std::thread::id thread_id;
	};
}
