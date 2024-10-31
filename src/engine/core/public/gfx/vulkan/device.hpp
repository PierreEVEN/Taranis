#pragma once
#include <unordered_map>

#include "physical_device.hpp"
#include "gfx/renderer/renderer_definition.hpp"

namespace Engine
{
	class RenderPassObject;
	class RenderPassInfos;
	class Queues;

	class Device : public std::enable_shared_from_this<Device>
	{
		friend class Engine;

	public:
		static std::shared_ptr<Device> create(const Config& config, const PhysicalDevice& physical_device, const Surface& surface);
		~Device();

		VkDevice raw() const { return ptr; }

		const Queues& get_queues() const;

		PhysicalDevice get_physical_device() const { return physical_device; }

		static const std::vector<const char*>& get_device_extensions();
		std::shared_ptr<RenderPassObject> find_or_create_render_pass(const RenderPassInfos& infos);

		void destroy_resources();
	private:
		Device(const Config& config, const PhysicalDevice& physical_device, const Surface& surface);
		std::unordered_map<RenderPassInfos, std::shared_ptr<RenderPassObject>> render_passes;
		std::unique_ptr<Queues> queues;
		PhysicalDevice physical_device;
		VkDevice ptr = VK_NULL_HANDLE;
	};
}
