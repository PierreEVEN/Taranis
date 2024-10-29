#include "gfx/vulkan/queue_family.hpp"

#include <unordered_map>

#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/instance.hpp"
#include "gfx/vulkan/physical_device.hpp"
#include "gfx/vulkan/surface.hpp"

namespace Engine
{
	Queues::Queues(const PhysicalDevice& physical_device)
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device.raw(), &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device.raw(), &queueFamilyCount, queueFamilies.data());

		uint32_t i = 0;

		for (const auto& queueFamily : queueFamilies)
		{
			auto queue = std::make_shared<QueueFamily>(i, queueFamily.queueFlags, false);
			all_queues.emplace_back(queue);
			i++;
		}
		update_specializations();
	}

	std::shared_ptr<QueueFamily> Queues::get_queue(QueueSpecialization specialization) const
	{
		auto val = preferred.find(static_cast<uint8_t>(specialization));
		return val == preferred.end() ? val->second : nullptr;
	}

	void Queues::init_first_surface(const Surface& surface, const PhysicalDevice& device)
	{
		for (const auto& queue : all_queues)
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device.raw(), queue->index(), surface.raw(), &presentSupport);
			if (presentSupport)
				queue->mark_as_present_queue();
		}

		update_specializations();
	}

	void Queues::update_specializations()
	{
		std::unordered_map<uint32_t, std::shared_ptr<QueueFamily>> queue_map;
		for (const auto& queueFamily : all_queues)
			queue_map.emplace(queueFamily->index(), queueFamily);

		const auto stored_map = queue_map;

		// Find fallback queues for transfer and compute
		std::shared_ptr<QueueFamily> compute_queue = find_best_suited_queue_family(
			queue_map, VK_QUEUE_COMPUTE_BIT, false, {});
		std::shared_ptr<QueueFamily> transfer_queue = find_best_suited_queue_family(
			queue_map, VK_QUEUE_TRANSFER_BIT, false, {});

		// Find graphic queue (ideally with present capability which should always be the case)
		std::shared_ptr<QueueFamily> graphic_queue = find_best_suited_queue_family(
			queue_map, VK_QUEUE_GRAPHICS_BIT, false, {VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT});

		// Find present queue that is not a dedicated compute queue ideally
		std::shared_ptr<QueueFamily> present_queue;

		if (graphic_queue)
		{
			auto no_graphic_queue_map = queue_map;
			no_graphic_queue_map.erase(graphic_queue->index());
			if (compute_queue)
				no_graphic_queue_map.erase(compute_queue->index());
			if (auto found_present_queue = find_best_suited_queue_family(no_graphic_queue_map, 0, true, {}))
				present_queue = found_present_queue;
			else
				const auto no_graphic_queue_map = queue_map;
			no_graphic_queue_map.erase(graphic_queue->index());
			present_queue = find_best_suited_queue_family(no_graphic_queue_map, 0, true, {});
		}
		else
			present_queue = find_best_suited_queue_family(queue_map, 0, true, {});


		if (graphic_queue)
			queue_map.erase(graphic_queue->index());
		if (present_queue)
			queue_map.erase(present_queue->index());

		// Search dedicated async compute queue
		std::shared_ptr<QueueFamily> async_compute_queue = find_best_suited_queue_family(
			queue_map, VK_QUEUE_COMPUTE_BIT, false, {});
		if (async_compute_queue) queue_map.erase(async_compute_queue->index());

		// Search dedicated transfer queue
		if (auto dedicated_transfer_queue = find_best_suited_queue_family(queue_map, VK_QUEUE_TRANSFER_BIT, false, {}))
		{
			queue_map.erase(dedicated_transfer_queue->index());
			transfer_queue = dedicated_transfer_queue;
		}

		preferred.clear();
		if (graphic_queue) preferred.emplace(static_cast<uint8_t>(QueueSpecialization::Graphic), graphic_queue);
		if (compute_queue) preferred.emplace(static_cast<uint8_t>(QueueSpecialization::Compute), compute_queue);
		if (async_compute_queue)
			preferred.emplace(static_cast<uint8_t>(QueueSpecialization::AsyncCompute),
			                  async_compute_queue);
		if (transfer_queue) preferred.emplace(static_cast<uint8_t>(QueueSpecialization::Transfer), transfer_queue);
		if (present_queue) preferred.emplace(static_cast<uint8_t>(QueueSpecialization::Present), present_queue);
	}

	QueueFamily::QueueFamily(uint32_t index, VkQueueFlags flags, bool support_present) : queue_index(index),
		queue_flags(flags),
		queue_support_present(support_present), ptr(nullptr)
	{
	}

	void QueueFamily::init_queue(const Device& device)
	{
		vkGetDeviceQueue(device.raw(), index(), 0, &ptr);
	}

	auto Queues::find_best_suited_queue_family(
		const std::unordered_map<uint32_t, std::shared_ptr<QueueFamily>>& available, VkQueueFlags required_flags,
		bool require_present,
		const std::vector<VkQueueFlags>& desired_queue_flags) -> std::shared_ptr<QueueFamily>
	{
		uint32_t high_score = 0;
		std::shared_ptr<QueueFamily> best_queue;
		for (const auto& [index, family] : available)
		{
			if (require_present && !family->support_present())
				continue;
			if (!(family->flags() & required_flags))
				continue;
			uint32_t score = 0;
			best_queue = family;
			uint32_t max_value = desired_queue_flags.size();
			for (int power = 0; power < desired_queue_flags.size(); ++power)
				if (family->flags() & desired_queue_flags[power])
					score += max_value - power;
			if (score > high_score)
			{
				high_score = score;
				best_queue = family;
			}
		}
		return best_queue;
	}
}
