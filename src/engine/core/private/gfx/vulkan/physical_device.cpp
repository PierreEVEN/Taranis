#include "gfx/vulkan/physical_device.hpp"

#include <map>

#include "config.hpp"
#include "gfx/vulkan/instance.hpp"

namespace Engine
{
	PhysicalDevice::PhysicalDevice(VkPhysicalDevice device) : ptr(device)
	{
	}

	PhysicalDevice::~PhysicalDevice()
	{
	}

	std::vector<PhysicalDevice> PhysicalDevice::get_all_physical_devices(const Instance& instance)
	{
		std::vector<PhysicalDevice> found_devices;
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance.raw(), &deviceCount, nullptr);
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance.raw(), &deviceCount, devices.data());
		for (const auto& device : devices)
			found_devices.emplace_back(device);
		return found_devices;
	}

	Result<PhysicalDevice> PhysicalDevice::pick_best_physical_device(const Instance& instance, const Config& config)
	{
		std::vector<PhysicalDevice> devices = get_all_physical_devices(instance);
		std::multimap<int32_t, PhysicalDevice> candidates;
		std::vector<std::string> errors;
		for (const auto& device : devices)
		{
			auto score = device.rate_device(config);
			if (score)
				candidates.insert(std::make_pair(score.get(), device));
			else
				errors.emplace_back(device.get_device_name() + " : " + score.error());
		}

		if (candidates.empty())
		{
			std::string message = "There is no suitable physical device :";
			for (const auto& error : errors)
				message += error + "\n";
			return Result<PhysicalDevice>::Error(message);
		}

		return Result<PhysicalDevice>::Ok(candidates.begin()->second);
	}

	Result<int32_t> PhysicalDevice::rate_device(const Config& config) const
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(ptr, &deviceProperties);
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(ptr, &deviceFeatures);

		if (config.allow_integrated_gpus && deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			return Result<int32_t>::Error(
				"The required device {} is an integrated GPU but integrated gpu are not currently allowed");

		int32_t score = 0;

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			score += 1000;
		}

		score += deviceProperties.limits.maxImageDimension2D;

		return Result<int32_t>::Ok(score);
	}

	std::string PhysicalDevice::get_device_name() const
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(ptr, &deviceProperties);
		return deviceProperties.deviceName;
	}
}
