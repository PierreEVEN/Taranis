#pragma once
#include <vulkan/vulkan_core.h>
#include <memory>
#include <vector>

namespace Engine {
	class Surface;

	class Device;

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class Swapchain
	{
	public:
		Swapchain(const std::weak_ptr<Device>& device, const std::weak_ptr<Surface>& Surface);
		~Swapchain();

		VkSwapchainKHR raw() const { return ptr; }


		static VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
		static VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes);
		static VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR& capabilities, VkExtent2D base_extent);

		void create_or_recreate();

		void render();

	private:

		void destroy();

		std::weak_ptr<Device> device;
		std::weak_ptr<Surface> surface;
		VkSwapchainKHR ptr = VK_NULL_HANDLE;
		VkFormat swapchain_format = VK_FORMAT_UNDEFINED;
		VkExtent2D extent = {};
		std::vector<VkImage> swapChainImages;
		std::vector<VkImageView> image_views;

	};
}
