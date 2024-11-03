#pragma once
#include <memory>
#include <glm/vec2.hpp>
#include <vulkan/vulkan_core.h>

#include "renderer_definition.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/framebuffer.hpp"


namespace Engine
{
	class Image;
	class ImageView;
	class InternalPassInstance;
	class SwapchainPresentPass;
	class RenderPassInstanceBase;
	class Device;

	class RenderPassObject
	{
	public:
		RenderPassObject(const std::shared_ptr<Device>& device, RenderPassInfos infos);
		~RenderPassObject();

		VkRenderPass raw() const { return ptr; }

		const RenderPassInfos& get_infos() const { return infos; }
		std::weak_ptr<Device> get_device() const { return device; }

	private:
		RenderPassInfos infos;
		std::weak_ptr<Device> device;
		VkRenderPass ptr = VK_NULL_HANDLE;
	};

	class RenderPassInstanceBase
	{
	public:
		RenderPassInstanceBase(std::shared_ptr<RenderPassObject> render_pass, std::shared_ptr<RenderPassInterface> interface);

		[[nodiscard]] const std::weak_ptr<RenderPassObject>& get_render_pass() const { return render_pass; }
		virtual std::vector<std::weak_ptr<ImageView>> get_attachments() const = 0;

		virtual void render(uint32_t output_framebuffer, uint32_t current_frame);

		virtual const Semaphore* get_wait_semaphores(uint32_t) const { return nullptr; }
		virtual const Fence* get_signal_fence(uint32_t) const { return nullptr; }


		void add_child_render_pass(std::shared_ptr<InternalPassInstance> child)
		{
			children.emplace_back(child);
		}

		virtual glm::uvec2 resolution() const = 0;

	protected:
		void new_frame_internal();
		std::shared_ptr<RenderPassInterface> interface;
		bool rendered = false;
		std::weak_ptr<Device> device;
		std::vector<std::shared_ptr<InternalPassInstance>> children;
		std::weak_ptr<RenderPassObject> render_pass;
		std::vector<std::shared_ptr<Framebuffer>> framebuffers;
	};

	class RenderPassRoot : public RenderPassInstanceBase
	{
	public:
		RenderPassRoot(std::shared_ptr<RenderPassObject> render_pass, std::shared_ptr<RendererStep> present_pass);
		void render(uint32_t output_framebuffer, uint32_t current_frame) override;
	};

	class SwapchainPresentPass : public RenderPassRoot
	{
	public:
		SwapchainPresentPass(std::shared_ptr<RenderPassObject> render_pass, std::weak_ptr<Swapchain> target,
		                     std::shared_ptr<RendererStep> present_step);
		~SwapchainPresentPass();
		std::vector<std::weak_ptr<ImageView>> get_attachments() const override;

		glm::uvec2 resolution() const override;
		void resize(glm::uvec2 parent_resolution);

		const Semaphore& get_render_finished_semaphore(uint32_t image_index) const;
		const Semaphore* get_wait_semaphores(uint32_t image_index) const override;
		const Fence* get_signal_fence(uint32_t image_index) const override;

	private:
		std::weak_ptr<Swapchain> swapchain;
	};

	class InternalPassInstance : public RenderPassInstanceBase
	{
	public:
		InternalPassInstance(std::shared_ptr<RenderPassObject> render_pass, std::shared_ptr<RenderPassInterface> interface);
		~InternalPassInstance();
		std::vector<std::weak_ptr<ImageView>> get_attachments() const override
		{
			std::vector<std::weak_ptr<ImageView>> attachments;
			for (const auto& attachment : framebuffer_image_views)
				attachments.emplace_back(attachment);
			return attachments;
		}

		glm::uvec2 resolution() const override
		{
			return framebuffer_resolution;
		}

		void resize(glm::uvec2 base_resolution);

	private:
		glm::uvec2 framebuffer_resolution;

		std::vector<std::shared_ptr<Image>> framebuffer_images;
		std::vector<std::shared_ptr<ImageView>> framebuffer_image_views;
	};
}
