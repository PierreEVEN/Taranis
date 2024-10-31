#include "gfx/renderer/renderer.hpp"

#include "gfx/vulkan/device.hpp"

namespace Engine
{
	RenderPassObject::RenderPassObject(const std::shared_ptr<Device>& in_device, RenderPassInfos in_infos) : infos(std::move(in_infos)), device(
		in_device)
	{
		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkAttachmentReference> color_attachment_references;
		std::optional<VkAttachmentReference> depth_attachment_references;

		for (const auto& attachment : infos.attachments)
		{
			if (attachment.is_depth())
				depth_attachment_references = VkAttachmentReference{
					.attachment = static_cast<uint32_t>(attachments.size()),
					.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				};
			else
				color_attachment_references.emplace_back(VkAttachmentReference{
					.attachment = static_cast<uint32_t>(attachments.size()),
					.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				});

			attachments.emplace_back(VkAttachmentDescription{
				.format = attachment.get_format(),
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = attachment.clear_value() ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = attachment.is_present_pass()
					               ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
					               : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			});
		}

		VkSubpassDescription subpass{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = static_cast<uint32_t>(attachments.size()),
			.pColorAttachments = color_attachment_references.data(),
			.pDepthStencilAttachment = depth_attachment_references ? &*depth_attachment_references : nullptr,
		};

		VkRenderPassCreateInfo renderPassInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = static_cast<uint32_t>(attachments.size()),
			.pAttachments = attachments.data(),
			.subpassCount = 1,
			.pSubpasses = &subpass,
		};

		VK_CHECK(vkCreateRenderPass(in_device->raw(), &renderPassInfo, nullptr, &ptr),
		         "Failed to create render pass")
	}

	RenderPassObject::~RenderPassObject()
	{
		vkDestroyRenderPass(device.lock()->raw(), ptr, nullptr);
	}

	Renderer::Renderer(const std::weak_ptr<Device>& device, const std::shared_ptr<RenderPass>& in_present_pass, Target target)
	{
		present_pass = std::make_shared<RenderPassInstance>(
			device.lock()->find_or_create_render_pass(in_present_pass->get_infos()));
	}

	RenderPassInstance::RenderPassInstance(std::shared_ptr<RenderPassObject> render_pass)
	{
	}
}
