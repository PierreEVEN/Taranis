#pragma once
#include <memory>
#include <glm/vec2.hpp>
#include <vulkan/vulkan_core.h>

#include "renderer_definition.hpp"
#include "gfx/vulkan/device.hpp"


namespace Engine
{
	class Framebuffer;
	class RenderPassInstance;
	class Device;

	class RenderPassObject
	{
	public:
		RenderPassObject(const std::shared_ptr<Device>& device, RenderPassInfos infos);
		~RenderPassObject();

		VkRenderPass raw() const { return ptr; }

		const RenderPassInfos& get_infos() const { return infos; }

	private:
		RenderPassInfos infos;
		std::weak_ptr<Device> device;
		VkRenderPass ptr = VK_NULL_HANDLE;
	};


	class Renderer
	{
	public:
		class Target
		{
		public:
			static Target swapchain(std::weak_ptr<Swapchain> window)
			{
				auto target = Target{};
				target.window_ref = std::move(window);
				return target;
			}

		private:
			Target() = default;
			std::weak_ptr<Swapchain> window_ref;
		};

		Renderer(const std::weak_ptr<Device>& device, const std::shared_ptr<RenderPass>& in_present_pass,
		         Target target);

	private:
		std::shared_ptr<RenderPassInstance> present_pass;
	};

	class RenderPassInstance
	{
	public:
		class Attachment
		{
		public:
			VkImageView get_view() const;
			glm::ivec2 get_resolution() const;
		};

		RenderPassInstance(std::shared_ptr<RenderPassObject> render_pass);

		[[nodiscard]] const std::weak_ptr<RenderPassObject>& get_render_pass() const { return render_pass; }
		[[nodiscard]] const std::vector<Attachment>& get_attachments() const { return attachments; }

	private:
		std::vector<Attachment> attachments;
		std::weak_ptr<RenderPassObject> render_pass;
		std::vector<std::shared_ptr<Framebuffer>> framebuffers;
	};
}
