#pragma once
#include <vulkan/vulkan_core.h>
#include <memory>
#include <vector>
#include <glm/vec2.hpp>

#include "gfx/renderer/renderer_definition.hpp"

namespace Engine
{
	class ImageView;
	class SwapchainPresentPass;
}

namespace Engine {
	class Renderer;
	class Surface;

	class Device;

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class Swapchain : public std::enable_shared_from_this<Swapchain>
	{
	public:
		Swapchain(const std::weak_ptr<Device>& device, const std::weak_ptr<Surface>& Surface);
		~Swapchain();

		VkSwapchainKHR raw() const { return ptr; }


		static VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
		static VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes);
		static glm::uvec2 choose_extent(const VkSurfaceCapabilitiesKHR& capabilities, glm::uvec2 base_extent);

		void create_or_recreate();

		void render();

		std::weak_ptr<Surface> get_surface() const { return surface; }

		void set_renderer(const std::shared_ptr<RendererStep>& present_step);

		std::weak_ptr<ImageView> get_image_view() const;

		glm::uvec2 get_extent() const { return extent; }

	private:

		void destroy();

		std::weak_ptr<Device> device;
		std::weak_ptr<Surface> surface;
		VkSwapchainKHR ptr = VK_NULL_HANDLE;
		ColorFormat swapchain_format = ColorFormat::UNDEFINED;
		glm::uvec2 extent = {};
		std::vector<VkImage> swapChainImages;
		std::shared_ptr<ImageView> image_view;
		std::shared_ptr<SwapchainPresentPass> renderer;
	};
}
