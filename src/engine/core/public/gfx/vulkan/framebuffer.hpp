#pragma once
#include <memory>
#include <vulkan/vulkan_core.h>

namespace Engine
{
	class RenderPassInstance;
	class Device;
}

namespace Engine
{
	class CommandBuffer;

	class Framebuffer
	{
	public:

		Framebuffer(std::weak_ptr<Device> device, const std::weak_ptr<RenderPassInstance>& render_pass, size_t image_index);
		~Framebuffer();

	private:

		std::weak_ptr<Device> device;
		VkFramebuffer ptr = VK_NULL_HANDLE;
		std::unique_ptr<CommandBuffer> command_buffer;
	};
}
