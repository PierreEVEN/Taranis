#pragma once
#include <memory>
#include <vulkan/vulkan_core.h>

#include "renderer_definition.hpp"


namespace Engine
{
	class Device;

	class Renderer
	{
		
	};

	class RenderPass
	{
	public:
		RenderPass(const std::shared_ptr<Device>& device, const RenderPassInfos& infos);
		~RenderPass();
	private:
		std::weak_ptr<Device> device;
		VkRenderPass ptr = VK_NULL_HANDLE;
	};

}
