#pragma once
#include "physical_device.hpp"

namespace Engine
{
	class Queues;

	class Device
	{
		friend class Engine;
	public:
		Device(const Config& config, const PhysicalDevice& physical_device);
		~Device();

		VkDevice raw() const { return ptr; }

		const Queues& get_queues() const { return *queues; }

		PhysicalDevice get_physical_device() const{ return physical_device; }

	private:
		std::unique_ptr<Queues> queues;

		PhysicalDevice physical_device;

		VkDevice ptr = VK_NULL_HANDLE;
	};
}
