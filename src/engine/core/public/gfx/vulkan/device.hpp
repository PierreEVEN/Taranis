#pragma once
#include <unordered_map>

#include "physical_device.hpp"

namespace Engine
{
	class RenderPassObject;
	class RenderPassInfos;
	class Queues;

	class Device : public std::enable_shared_from_this<Device>
	{
		friend class Engine;
	public:
		Device(const Config& config, const PhysicalDevice& physical_device);
		~Device();

		VkDevice raw() const { return ptr; }

		const Queues& get_queues() const { return *queues; }

		PhysicalDevice get_physical_device() const{ return physical_device; }


		static const std::vector<const char*>& get_device_extensions();

		void declare_render_pass(const RenderPassInfos& render_pass_infos);

		std::shared_ptr<RenderPassObject> get_render_pass(const std::string& render_pass);


	private:

		std::unordered_map<std::string, RenderPassInfos> render_passes_declarations;
		std::unordered_map<std::string, std::shared_ptr<RenderPassObject>> render_passes;

		std::unique_ptr<Queues> queues;

		PhysicalDevice physical_device;

		VkDevice ptr = VK_NULL_HANDLE;
	};
}
