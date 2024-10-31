#pragma once
#include <mutex>
#include <unordered_map>
#include <vulkan/vulkan_core.h>
#include <optional>

namespace Engine
{
	class CommandPool;
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
	const char* get_queue_specialization_name(QueueSpecialization elem);

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
		void init_queue(const std::weak_ptr<Device>& device);

	private:
		uint32_t queue_index;
		VkQueueFlags queue_flags;
		bool queue_support_present;
		std::mutex queue_lock;
		VkQueue ptr = VK_NULL_HANDLE;
		std::unique_ptr<CommandPool> command_pool;
	};

	class Queues
	{
	public:
		Queues(const PhysicalDevice& physical_device, const Surface& surface);

		std::shared_ptr<QueueFamily> get_queue(QueueSpecialization specialization) const;

		std::vector<std::shared_ptr<QueueFamily>> all_families() const { return all_queues; }

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
