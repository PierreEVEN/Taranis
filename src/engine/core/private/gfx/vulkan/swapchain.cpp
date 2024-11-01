#include "gfx/vulkan/swapchain.hpp"

#include "gfx/window.hpp"
#include "gfx/renderer/renderer.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/queue_family.hpp"
#include "gfx/vulkan/surface.hpp"

namespace Engine
{
	Swapchain::Swapchain(const std::weak_ptr<Device>& in_device, const std::weak_ptr<Surface>& in_surface) : surface(
		in_surface), device(in_device)
	{
		create_or_recreate();
	}

	Swapchain::~Swapchain()
	{
		destroy();
	}

	VkSurfaceFormatKHR Swapchain::choose_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats)
	{
		for (const auto& availableFormat : available_formats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace ==
				VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		return available_formats[0];
	}

	VkPresentModeKHR Swapchain::choose_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes)
	{
		for (const auto& availablePresentMode : available_present_modes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	glm::uvec2 Swapchain::choose_extent(const VkSurfaceCapabilitiesKHR& capabilities, glm::uvec2 base_extent)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return glm::uvec2{capabilities.currentExtent.width, capabilities.currentExtent.height};
		}
		base_extent.x = std::clamp(static_cast<uint32_t>(base_extent.x), capabilities.minImageExtent.width,
		                           capabilities.maxImageExtent.width);
		base_extent.y = std::clamp(static_cast<uint32_t>(base_extent.y), capabilities.minImageExtent.height,
		                           capabilities.maxImageExtent.height);

		return base_extent;
	}

	void Swapchain::create_or_recreate()
	{
		destroy();

		const auto device_ptr = device.lock();

		SwapChainSupportDetails swapchain_support = device_ptr->get_physical_device().query_swapchain_support(
			*surface.lock());

		VkSurfaceFormatKHR surfaceFormat = choose_surface_format(swapchain_support.formats);
		swapchain_format = static_cast<ColorFormat>(surfaceFormat.format);
		VkPresentModeKHR presentMode = choose_present_mode(swapchain_support.presentModes);
		const auto window_extent = surface.lock()->get_window().lock()->internal_extent();
		extent = choose_extent(swapchain_support.capabilities, window_extent);

		uint32_t imageCount = swapchain_support.capabilities.minImageCount + 1;

		if (swapchain_support.capabilities.maxImageCount > 0 && imageCount > swapchain_support.capabilities.
			maxImageCount)
		{
			imageCount = swapchain_support.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = surface.lock()->raw(),
			.minImageCount = imageCount,
			.imageFormat = surfaceFormat.format,
			.imageColorSpace = surfaceFormat.colorSpace,
			.imageExtent = VkExtent2D{static_cast<uint32_t>(extent.x), static_cast<uint32_t>(extent.y)},
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.preTransform = swapchain_support.capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = presentMode,
			.clipped = VK_TRUE,
			.oldSwapchain = VK_NULL_HANDLE,
		};

		const uint32_t graphic = device_ptr->get_queues().get_queue(QueueSpecialization::Graphic)->index();
		const uint32_t present = device_ptr->get_queues().get_queue(QueueSpecialization::Present)->index();
		uint32_t queueFamilyIndices[] = {graphic, present};

		if (graphic != present)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		VK_CHECK(vkCreateSwapchainKHR(device_ptr->raw(), &createInfo, nullptr, &ptr), "Failed to create swapchain")


		vkGetSwapchainImagesKHR(device_ptr->raw(), ptr, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device_ptr->raw(), ptr, &imageCount, swapChainImages.data());

		image_view = std::make_shared<ImageView>(device, swapChainImages,
		                                         ImageView::CreateInfos{.format = swapchain_format});
	}

	void Swapchain::render()
	{
	}

	void Swapchain::destroy()
	{
		image_view = nullptr;

		if (ptr != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(device.lock()->raw(), ptr, nullptr);
			ptr = VK_NULL_HANDLE;
		}
	}

	void Swapchain::set_renderer(const std::shared_ptr<RendererStep>& present_step)
	{
		renderer = std::make_shared<SwapchainPresentPass>(
			device.lock()->find_or_create_render_pass(present_step->get_infos()), weak_from_this(), present_step);
	}

	std::weak_ptr<ImageView> Swapchain::get_image_view() const
	{
		return image_view;
	}
}
