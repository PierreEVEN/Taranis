#include "gfx/renderer/renderer.hpp"

#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/image.hpp"
#include "gfx/vulkan/image_view.hpp"

namespace Engine
{
	RenderPassObject::RenderPassObject(const std::shared_ptr<Device>& in_device, RenderPassInfos in_infos) :
		infos(std::move(in_infos)), device(
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
				.format = static_cast<VkFormat>(attachment.get_format()),
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
			.colorAttachmentCount = static_cast<uint32_t>(attachments.size()) - (depth_attachment_references ? 1 : 0),
			.pColorAttachments = color_attachment_references.data(),
			.pDepthStencilAttachment = depth_attachment_references ? &*depth_attachment_references : nullptr,
		};

		const std::array dependencies{
			VkSubpassDependency{
				.srcSubpass = VK_SUBPASS_EXTERNAL, // Producer of the dependency
				.dstSubpass = 0, // Consumer is our single subpass that will wait for the execution depdendency
				.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				// Match our pWaitDstStageMask when we vkQueueSubmit
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				// is a loadOp stage for color color_attachments
				.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT, // semaphore wait already does memory dependency for us
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				// is a loadOp CLEAR access mask for color color_attachments
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
			},
			VkSubpassDependency{
				.srcSubpass = 0, // Producer of the dependency is our single subpass
				.dstSubpass = VK_SUBPASS_EXTERNAL, // Consumer are all commands outside of the renderpass
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				// is a storeOp stage for color color_attachments
				.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // Do not block any subsequent work
				.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				// is a storeOp `STORE` access mask for color color_attachments
				.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
			}
		};

		VkRenderPassCreateInfo renderPassInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = static_cast<uint32_t>(attachments.size()),
			.pAttachments = attachments.data(),
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = static_cast<uint32_t>(dependencies.size()),
			.pDependencies = dependencies.data(),
		};

		VK_CHECK(vkCreateRenderPass(in_device->raw(), &renderPassInfo, nullptr, &ptr),
		         "Failed to create render pass")
	}

	RenderPassObject::~RenderPassObject()
	{
		vkDestroyRenderPass(device.lock()->raw(), ptr, nullptr);
	}

	RenderPassInstanceBase::RenderPassInstanceBase(std::shared_ptr<RenderPassObject> in_render_pass) : render_pass(
			in_render_pass), device(in_render_pass->get_device())
	{
	}

	void RenderPassInstanceBase::render()
	{
		rendered = true;

		for (const auto& child : children)
			child->render();
	}

	void RenderPassInstanceBase::new_frame_internal()
	{
		rendered = false;
		for (auto& child : children)
			child->new_frame_internal();
	}

	SwapchainPresentPass::SwapchainPresentPass(std::shared_ptr<RenderPassObject> render_pass,
	                                           std::weak_ptr<Swapchain> in_target,
	                                           std::shared_ptr<RendererStep> present_step): RenderPassRoot(
			render_pass, present_step), swapchain(in_target)
	{
		resize(swapchain.lock()->get_extent());
	}

	std::vector<std::weak_ptr<ImageView>> SwapchainPresentPass::get_attachments() const
	{
		return { swapchain.lock()->get_image_view() };
	}

	glm::uvec2 SwapchainPresentPass::resolution() const
	{
		return swapchain.lock()->get_extent();
	}

	void SwapchainPresentPass::resize(glm::uvec2 parent_resolution)
	{
		for (auto& child : children)
			child->resize(parent_resolution);

		int image_index = 0;
		framebuffers.clear();
		for (const auto& _ : swapchain.lock()->get_image_view().lock()->raw())
			framebuffers.emplace_back(render_pass.lock()->get_device(), *this, image_index++);
	}

	InternalPassInstance::InternalPassInstance(std::shared_ptr<RenderPassObject> render_pass)
		: RenderPassInstanceBase(render_pass)
	{
	}

	void InternalPassInstance::resize(glm::uvec2 base_resolution)
	{
		for (auto& child : children)
			child->resize(base_resolution);
		if (framebuffer_resolution == base_resolution)
			return;
		framebuffer_resolution = base_resolution;

		framebuffer_images.clear();
		framebuffer_image_views.clear();
		for (const auto& attachment : render_pass.lock()->get_infos().attachments)
		{
			const auto image = std::make_shared<Image>(device, ImageParameter{
				                                           .format = attachment.get_format(),
				                                           .gpu_write_capabilities =
				                                           ETextureGPUWriteCapabilities::Enabled,
				                                           .width = framebuffer_resolution.x,
				                                           .height = framebuffer_resolution.y,
			                                           });
			framebuffer_images.emplace_back();
			framebuffer_image_views.emplace_back(std::make_shared<ImageView>(image, ImageView::CreateInfos{
				                                                                 .format = attachment.get_format()
			                                                                 }));
		}
	}

	RenderPassRoot::RenderPassRoot(std::shared_ptr<RenderPassObject> render_pass_object,
	                               std::shared_ptr<RendererStep> present_pass): RenderPassInstanceBase(
		render_pass_object)
	{
		// Instantiate all unique render passes
		std::unordered_map<std::shared_ptr<RendererStep>, std::shared_ptr<InternalPassInstance>> instanced_passes;
		std::vector remaining = {present_pass};
		while (!remaining.empty())
		{
			const std::shared_ptr<RendererStep> def = remaining.back();
			remaining.pop_back();
			instanced_passes.emplace(def, std::make_shared<InternalPassInstance>(
				                         render_pass_object->get_device().lock()->find_or_create_render_pass(
					                         def->get_infos())));
			for (const auto& dep : def->get_dependencies())
				remaining.emplace_back(dep);
		}

		// Construct render pass tree
		remaining.emplace_back(present_pass);
		while (!remaining.empty())
		{
			const std::shared_ptr<RendererStep> def = remaining.back();
			remaining.pop_back();

			for (const auto& dep : def->get_dependencies())
			{
				instanced_passes.find(def)->second->add_child_render_pass(instanced_passes.find(dep)->second);
				remaining.emplace_back(dep);
			}
		}
	}

	void RenderPassRoot::render()
	{
		new_frame_internal();
		RenderPassInstanceBase::render();
	}
}
