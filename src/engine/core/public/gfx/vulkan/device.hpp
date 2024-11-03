#pragma once
#include <unordered_map>
#include "vk_mem_alloc.h"

#include "physical_device.hpp"
#include "gfx/renderer/renderer_definition.hpp"

namespace Engine
{
	class DescriptorPool;
}

namespace Engine
{
	class DeviceResource;
	class RenderPassObject;
	class RenderPassInfos;
	class Queues;

	class Device : public std::enable_shared_from_this<Device>
	{
		friend class Engine;

	public:
		static std::shared_ptr<Device> create(const Config& config, const Instance& instance,
		                                      const PhysicalDevice& physical_device, const Surface& surface);
		~Device();

		VkDevice raw() const { return ptr; }

		const Queues& get_queues() const;
		const VmaAllocator& get_allocator() const { return allocator; }

		PhysicalDevice get_physical_device() const { return physical_device; }

		static const std::vector<const char*>& get_device_extensions();
		std::shared_ptr<RenderPassObject> find_or_create_render_pass(const RenderPassInfos& infos);

		void destroy_resources();

		uint32_t get_image_count() const { return image_count; }
		uint32_t get_current_image() const { return current_image; }

		void next_frame();

		void wait() const;

		void drop_resource(const std::shared_ptr<DeviceResource>& resource)
		{
			std::lock_guard lock(resource_mutex);
			if (current_image > pending_kill_resources.size())
				pending_kill_resources.resize(current_image);
			pending_kill_resources[current_image].emplace_back(resource);
		}

		DescriptorPool& get_descriptor_pool() const { return *descriptor_pool; }

	private:
		Device(const Config& config, const Instance& instance, const PhysicalDevice& physical_device,
		       const Surface& surface);
		std::unordered_map<RenderPassInfos, std::shared_ptr<RenderPassObject>> render_passes;
		std::unique_ptr<Queues> queues;
		PhysicalDevice physical_device;
		VkDevice ptr = VK_NULL_HANDLE;
		VmaAllocator allocator;
		uint32_t image_count = 2;
		uint32_t current_image = 0;
		std::mutex resource_mutex;
		std::vector<std::vector<std::shared_ptr<DeviceResource>>> pending_kill_resources;
		std::shared_ptr<DescriptorPool> descriptor_pool;
	};

	class DeviceResource : public std::enable_shared_from_this<DeviceResource>
	{
	public:
		DeviceResource(std::weak_ptr<Device> in_device) : device_ref(std::move(in_device))
		{
		}

		const std::weak_ptr<Device>& device() const { return device_ref; }

	private:
		std::weak_ptr<Device> device_ref;
	};
}
