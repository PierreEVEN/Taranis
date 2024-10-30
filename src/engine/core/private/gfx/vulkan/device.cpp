#include "gfx/vulkan/device.hpp"

#include "config.hpp"
#include "gfx/renderer/renderer.hpp"
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

	void Device::declare_render_pass(const std::string& render_pass, const RenderPassInfos& render_pass_infos)
	{
		render_passes_declarations.emplace(render_pass, render_pass_infos);
	}

	std::shared_ptr<RenderPass> Device::get_render_pass(const std::string& render_pass)
	{
		const auto existing = render_passes.find(render_pass);

		if (existing != render_passes.end())
			return existing->second;

		const auto infos = render_passes_declarations.find(render_pass);

		if (infos != render_passes_declarations.end()) {
			const auto new_render_pass = std::make_shared<RenderPass>(shared_from_this(), infos->second);
			render_passes.emplace(render_pass, new_render_pass);
			return new_render_pass;
		}
		return nullptr;

	}
}
