#pragma once
#include <memory>
#include <vulkan/vulkan_core.h>

namespace Engine
{
	class RenderPassInstanceBase;
	class Device;
}

namespace Engine
{
	class CommandBuffer;

	class Framebuffer
	{
	public:

		Framebuffer(std::weak_ptr<Device> device, const RenderPassInstanceBase& render_pass, size_t image_index);
		~Framebuffer();

	private:

		std::weak_ptr<Device> device;
		VkFramebuffer ptr = VK_NULL_HANDLE;
		std::shared_ptr<CommandBuffer> command_buffer;
	};
}
