#include "gfx/vulkan/framebuffer.hpp"

#include "gfx/renderer/renderer.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/device.hpp"

namespace Engine
{
	Framebuffer::Framebuffer(std::weak_ptr<Device> in_device, const std::weak_ptr<RenderPassInstance>& render_pass,
	                         size_t image_index) : device(std::move(in_device)), command_buffer(std::make_unique<CommandBuffer>(device))
	{
		const auto rp_instance = render_pass.lock();
		const auto rp = rp_instance->get_render_pass().lock();

		std::vector<VkImageView> views;
		for (const auto& view : rp_instance->get_attachments())
			views.emplace_back(view.get_view());


		VkFramebufferCreateInfo create_infos = {
			.renderPass = rp->raw(),
			.attachmentCount = static_cast<uint32_t>(views.size()),
			.pAttachments = views.data(),
			.width = static_cast<uint32_t>(rp_instance->get_attachments()[0].get_resolution().x),
			.height = static_cast<uint32_t>(rp_instance->get_attachments()[0].get_resolution().y),
			.layers = 1,
		};

		VK_CHECK(vkCreateFramebuffer(in_device.lock()->raw(), &create_infos, nullptr, &ptr),
		         "Failed to create render pass");
	}

	Framebuffer::~Framebuffer()
	{
		vkDestroyFramebuffer(device.lock()->raw(), ptr, nullptr);
	}
}
