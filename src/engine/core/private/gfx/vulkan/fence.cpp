#include "gfx/vulkan/fence.hpp"

#include "gfx/vulkan/device.hpp"

namespace Engine
{
	Fence::Fence(std::weak_ptr<Device> in_device) : device(std::move(in_device))
	{
		VkFenceCreateInfo create_infos = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};

		VK_CHECK(vkCreateFence(device.lock()->raw(), &create_infos, nullptr, &ptr), "Failed to create fence");
	}

	Fence::~Fence()
	{
		vkDestroyFence(device.lock()->raw(), ptr, nullptr);
	}

	void Fence::reset() const
	{
		VK_CHECK(vkResetFences(device.lock()->raw(), 1, &ptr), "Failed to reset fence");
	}

	void Fence::wait() const
	{
		VK_CHECK(vkWaitForFences(device.lock()->raw(), 1, &ptr, true, UINT64_MAX), "Failed to wait for fence");
	}
}
