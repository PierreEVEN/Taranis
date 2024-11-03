#include "gfx/renderer/renderer.hpp"

#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/image.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/semaphore.hpp"

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
				.loadOp = attachment.clear_value().is_none()
					          ? VK_ATTACHMENT_LOAD_OP_DONT_CARE
					          : VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = infos.present_pass
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

	RenderPassInstanceBase::RenderPassInstanceBase(std::shared_ptr<RenderPassObject> in_render_pass,
	                                               std::shared_ptr<RenderPassInterface> in_interface) : render_pass(
		in_render_pass), device(in_render_pass->get_device()), interface(in_interface)
	{
		interface->init(device, *this);
	}

	void RenderPassInstanceBase::render(uint32_t output_framebuffer, uint32_t current_frame)
	{
		rendered = true;
		for (const auto& child : children)
			child->render(current_frame, current_frame);

		// Begin get record
		const auto& framebuffer = framebuffers[output_framebuffer];
		framebuffer->get_command_buffer().begin(false);

		// Begin render pass
		std::vector<VkClearValue> clear_values;
		for (auto& attachment : render_pass.lock()->get_infos().attachments)
		{
			VkClearValue clear_value;
			clear_value.color = {0, 0, 0, 1};
			if (attachment.is_depth())
				clear_value.depthStencil = {0, 0};
			if (attachment.clear_value().is_color())
				clear_value.color = VkClearColorValue{
					.float32 = {
						attachment.clear_value().color().x, attachment.clear_value().color().y,
						attachment.clear_value().color().z, attachment.clear_value().color().w
					}
				};
			else if (attachment.clear_value().is_depth_stencil())
				clear_value.depthStencil = VkClearDepthStencilValue{
					attachment.clear_value().depth_stencil().x,
					static_cast<uint32_t>(attachment.clear_value().depth_stencil().y)
				};
			clear_values.emplace_back(clear_value);
		}

		const VkRenderPassBeginInfo begin_infos = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = render_pass.lock()->raw(),
			.framebuffer = framebuffer->raw(),
			.renderArea =
			{
				.offset = {0, 0},
				.extent = {resolution().x, resolution().y},
			},
			.clearValueCount = static_cast<uint32_t>(clear_values.size()),
			.pClearValues = clear_values.data(),
		};
		vkCmdBeginRenderPass(framebuffer->get_command_buffer().raw(), &begin_infos, VK_SUBPASS_CONTENTS_INLINE);

		// Set viewport and scissor
		const VkViewport viewport{
			.x = 0,
			.y = static_cast<float>(resolution().y),
			// Flip viewport vertically to avoid textures to being displayed upside down
			.width = static_cast<float>(resolution().x),
			.height = -static_cast<float>(resolution().y),
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};
		vkCmdSetViewport(framebuffer->get_command_buffer().raw(), 0, 1, &viewport);

		const VkRect2D scissor{
			.offset = VkOffset2D{0, 0},
			.extent = VkExtent2D{resolution().x, resolution().y},
		};
		vkCmdSetScissor(framebuffer->get_command_buffer().raw(), 0, 1, &scissor);

		interface->render(*this, framebuffer->get_command_buffer());

		// End command get
		vkCmdEndRenderPass(framebuffer->get_command_buffer().raw());

		framebuffer->get_command_buffer().end();

		// Submit get (wait children completion using children_semaphores)
		std::vector<VkSemaphore> children_semaphores;
		for (const auto& child : children)
			children_semaphores.emplace_back(child->framebuffers[current_frame]->render_finished_semaphore().raw());
		if (const auto& wait_semaphore = get_wait_semaphores(current_frame))
			children_semaphores.emplace_back(wait_semaphore->raw());

		std::vector<VkPipelineStageFlags> wait_stage(children_semaphores.size(),
		                                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
		const auto command_buffer_ptr = framebuffer->get_command_buffer().raw();
		const auto render_finished_semaphore_ptr = framebuffer->render_finished_semaphore().raw();
		const VkSubmitInfo submit_infos{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = static_cast<uint32_t>(children_semaphores.size()),
			.pWaitSemaphores = children_semaphores.data(),
			.pWaitDstStageMask = wait_stage.data(),
			.commandBufferCount = 1,
			.pCommandBuffers = &command_buffer_ptr,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &render_finished_semaphore_ptr,
		};

		framebuffer->get_command_buffer().submit(submit_infos, get_signal_fence(current_frame));
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

	SwapchainPresentPass::~SwapchainPresentPass()
	{
		children.clear();
	}

	std::vector<std::weak_ptr<ImageView>> SwapchainPresentPass::get_attachments() const
	{
		return {swapchain.lock()->get_image_view()};
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
		const size_t image_count = swapchain.lock()->get_image_view().lock()->raw().size();
		for (size_t i = 0; i < image_count; ++i)
			framebuffers.emplace_back(
				std::make_shared<Framebuffer>(render_pass.lock()->get_device(), *this, image_index++));
	}

	const Semaphore& SwapchainPresentPass::get_render_finished_semaphore(uint32_t image_index) const
	{
		return framebuffers[image_index]->render_finished_semaphore();
	}

	const Semaphore* SwapchainPresentPass::get_wait_semaphores(uint32_t image_index) const
	{
		return &swapchain.lock()->get_image_available_semaphore(image_index);
	}

	const Fence* SwapchainPresentPass::get_signal_fence(uint32_t image_index) const
	{
		return &swapchain.lock()->get_in_flight_fence(image_index);
	}

	InternalPassInstance::InternalPassInstance(std::shared_ptr<RenderPassObject> render_pass,
	                                           std::shared_ptr<RenderPassInterface> interface)
		: RenderPassInstanceBase(render_pass, interface)
	{
	}

	InternalPassInstance::~InternalPassInstance()
	{
		framebuffer_images.clear();
		framebuffer_image_views.clear();
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
				.buffer_type = EBufferType::IMMEDIATE,
				                                           .width = framebuffer_resolution.x,
				                                           .height = framebuffer_resolution.y,
			                                           });
			framebuffer_images.emplace_back();
			framebuffer_image_views.emplace_back(std::make_shared<ImageView>(image));
		}
		framebuffers.clear();
		for (size_t i = 0; i < framebuffer_image_views[0]->raw().size(); ++i)
			framebuffers.emplace_back(std::make_shared<Framebuffer>(render_pass.lock()->get_device(), *this, i));
	}

	RenderPassRoot::RenderPassRoot(std::shared_ptr<RenderPassObject> render_pass_object,
	                               std::shared_ptr<RendererStep> present_pass): RenderPassInstanceBase(
		render_pass_object, present_pass->get_interface())
	{
		// Instantiate all unique render passes
		std::unordered_map<std::shared_ptr<RendererStep>, std::shared_ptr<InternalPassInstance>> instanced_passes;
		std::vector<std::shared_ptr<RendererStep>> remaining;
		for (const auto& dep : present_pass->get_dependencies())
			remaining.push_back(dep);
		while (!remaining.empty())
		{
			const std::shared_ptr<RendererStep> def = remaining.back();
			remaining.pop_back();
			instanced_passes.emplace(def, std::make_shared<InternalPassInstance>(
				                         render_pass_object->get_device().lock()->find_or_create_render_pass(
					                         def->get_infos()), def->get_interface()));
			for (const auto& dep : def->get_dependencies())
				remaining.emplace_back(dep);
		}

		// Construct render pass tree
		remaining.push_back(present_pass);
		while (!remaining.empty())
		{
			const std::shared_ptr<RendererStep> def = remaining.back();
			remaining.pop_back();

			for (const auto& dep : def->get_dependencies())
			{
				auto instanced_parent = instanced_passes.find(def);

				if (instanced_parent != instanced_passes.end())
					instanced_parent->second->add_child_render_pass(instanced_passes.find(dep)->second);
				else
					add_child_render_pass(instanced_passes.find(dep)->second);
				remaining.emplace_back(dep);
			}
		}
	}

	void RenderPassRoot::render(uint32_t output_framebuffer, uint32_t current_frame)
	{
		new_frame_internal();
		RenderPassInstanceBase::render(output_framebuffer, current_frame);
	}
}
