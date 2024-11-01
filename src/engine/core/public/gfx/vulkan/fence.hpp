#pragma once
#include <memory>
#include <vulkan/vulkan_core.h>

namespace Engine
{
	class Device;

	class Fence
	{
	public:
		Fence(std::weak_ptr<Device> device);
		~Fence();

		VkFence raw() const { return ptr; }

		void reset() const;
		void wait() const;

	private:
		VkFence ptr;
		std::weak_ptr<Device> device;
	};
}
