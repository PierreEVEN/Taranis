#include "gfx/vulkan/device.hpp"

#include "config.hpp"
#include "gfx/vulkan/instance.hpp"
#include "gfx/vulkan/queue_family.hpp"

namespace Engine
{
	Device::Device(const Config& config, const PhysicalDevice& physical_device) :
		queues(std::make_unique<Queues>(physical_device)), physical_device(physical_device)
	{
		float queuePriority = 1.0f;

		std::vector<VkDeviceQueueCreateInfo> queues_info;
		for (const auto& queue : queues->all_families())
		{
			queues_info.emplace_back(VkDeviceQueueCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = queue->index(),
				.queueCount = 1,
				.pQueuePriorities = &queuePriority
			});
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queues_info.data();
		createInfo.queueCreateInfoCount = queues_info.size();

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = 0;

		if (config.enable_validation_layers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(Instance::validation_layers().size());
			createInfo.ppEnabledLayerNames = Instance::validation_layers().data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		VK_CHECK(vkCreateDevice(physical_device.raw(), &createInfo, nullptr, &ptr), "Failed to create device");

		for (const auto& queue : queues->all_families()) {
			queue->init_queue(*this);
		}
	}

	Device::~Device()
	{
		vkDestroyDevice(ptr, nullptr);
	}
}
