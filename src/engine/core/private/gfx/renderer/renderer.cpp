#include "gfx/renderer/renderer.hpp"

#include "gfx/vulkan/device.hpp"

namespace Engine
{
	RenderPass::RenderPass(const std::shared_ptr<Device>& in_device, const RenderPassInfos& infos) : device(in_device)
	{
		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkAttachmentReference> color_attachment_references;
		VkAttachmentReference depth_attachment_references;

		for (const auto& attachment : infos.color_attachments)
		{
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
		};

		if (infos.depth_attachments)
		{
			depth_attachment_references = VkAttachmentReference{
				.attachment = static_cast<uint32_t>(attachments.size()),
				.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			};
			attachments.emplace_back(VkAttachmentDescription{
				.format = infos.depth_attachments->get_format(),
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = infos.depth_attachments->clear_value()
					          ? VK_ATTACHMENT_LOAD_OP_CLEAR
					          : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = infos.depth_attachments->is_present_pass()
					               ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
					               : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			});
			subpass.pDepthStencilAttachment = &depth_attachment_references;
		}

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

	RenderPass::~RenderPass()
	{
		vkDestroyRenderPass(device.lock()->raw(), ptr, nullptr);
	}
}
