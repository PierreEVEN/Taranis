#include "gfx/vulkan/swapchain.hpp"

#include "gfx/window.hpp"
#include "gfx/vulkan/device.hpp"
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

	VkExtent2D Swapchain::choose_extent(const VkSurfaceCapabilitiesKHR& capabilities, VkExtent2D base_extent)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		base_extent.width = std::clamp(base_extent.width, capabilities.minImageExtent.width,
		                               capabilities.maxImageExtent.width);
		base_extent.height = std::clamp(base_extent.height, capabilities.minImageExtent.height,
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
		swapchain_format = surfaceFormat.format;
		VkPresentModeKHR presentMode = choose_present_mode(swapchain_support.presentModes);
		const auto window_extent = surface.lock()->get_window().lock()->internal_extent();
		extent = choose_extent(swapchain_support.capabilities,
		                       VkExtent2D{
			                       static_cast<uint32_t>(window_extent.x),
			                       static_cast<uint32_t>(window_extent.y)
		                       });

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
			.imageExtent = extent,
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

		image_views.resize(swapChainImages.size());
		VkImageViewCreateInfo image_view_infos{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = swapchain_format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			}
		};
		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			image_view_infos.image = swapChainImages[i];
			VK_CHECK(vkCreateImageView(device_ptr->raw(), &image_view_infos, nullptr, &image_views[i]),
			         "failed to create swapchain image views");
		}
	}

	void Swapchain::render()
	{
	}

	void Swapchain::destroy()
	{
		for (const auto& view : image_views)
			vkDestroyImageView(device.lock()->raw(), view, nullptr);
		image_views.clear();

		if (ptr != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(device.lock()->raw(), ptr, nullptr);
			ptr = VK_NULL_HANDLE;
		}
	}
}
