#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#include "vk_check.hpp"


namespace Engine
{
	class Config;
	class Instance;

	class PhysicalDevice
	{
	public:
		PhysicalDevice(VkPhysicalDevice device);
		~PhysicalDevice();

		static std::vector<PhysicalDevice> get_all_physical_devices(const Instance& instance);
		static Result<PhysicalDevice> pick_best_physical_device(const Instance& instance, const Config& config);

		Result<int32_t> rate_device(const Config& config) const;

		std::string get_device_name() const;

		VkPhysicalDevice raw() const { return ptr; }

	private:

		VkPhysicalDevice ptr = VK_NULL_HANDLE;
	};
}
