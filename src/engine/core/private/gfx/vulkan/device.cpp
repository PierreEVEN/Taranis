#include "gfx/vulkan/device.hpp"

#include "config.hpp"
#include "gfx/vulkan/instance.hpp"
#include "gfx/vulkan/queue_family.hpp"

namespace Engine
{
	const std::vector device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

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
		VkDeviceCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount = static_cast<uint32_t>(queues_info.size()),
			.pQueueCreateInfos = queues_info.data(),
			.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
			.ppEnabledExtensionNames = device_extensions.data(),
			.pEnabledFeatures = &deviceFeatures,
		};

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

		for (const auto& queue : queues->all_families())
		{
			queue->init_queue(*this);
		}
	}

	Device::~Device()
	{
		vkDestroyDevice(ptr, nullptr);
	}

	const std::vector<const char*>& Device::get_device_extensions()
	{
		return device_extensions;
	}
}
