#pragma once
#include <mutex>
#include <unordered_map>
#include <vulkan/vulkan_core.h>
#include <optional>

namespace Engine
{
	class Surface;
	class Device;
}

namespace Engine
{
	class PhysicalDevice;
	class Instance;


	enum class QueueSpecialization : uint8_t
	{
		Graphic,
		Present,
		Transfer,
		Compute,
		AsyncCompute
	};

	static bool operator==(const QueueSpecialization& A, const QueueSpecialization& B)
	{
		return static_cast<uint8_t>(A) == static_cast<uint8_t>(B);
	}


	class QueueFamily
	{
	public:
		QueueFamily(uint32_t index, VkQueueFlags flags, bool support_present);

		bool support_present() const { return queue_support_present; }

		VkQueueFlags flags() const { return queue_flags; }
		uint32_t index() const { return queue_index; }
		void init_queue(const Device& device);
		void mark_as_present_queue() { queue_support_present = true; };

	private:
		uint32_t queue_index;
		VkQueueFlags queue_flags;
		bool queue_support_present;
		std::mutex queue_lock;
		VkQueue ptr = VK_NULL_HANDLE;
	};

	class Queues
	{
	public:
		Queues(const PhysicalDevice& physical_device);

		std::shared_ptr<QueueFamily> get_queue(QueueSpecialization specialization) const;

		std::vector<std::shared_ptr<QueueFamily>> all_families() const { return all_queues; }

		void init_first_surface(const Surface& surface, const PhysicalDevice& device);

	private:
		void update_specializations();

		std::unordered_map<uint8_t, std::shared_ptr<QueueFamily>> preferred;
		std::vector<std::shared_ptr<QueueFamily>> all_queues;

		static std::shared_ptr<QueueFamily> find_best_suited_queue_family(
			const std::unordered_map<uint32_t, std::shared_ptr<QueueFamily>>& available,
			VkQueueFlags required_flags,
			bool require_present, const std::vector<VkQueueFlags>& desired_queue_flags);
	};
}
