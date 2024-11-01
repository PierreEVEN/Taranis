#include "gfx/vulkan/swapchain.hpp"

#include "gfx/window.hpp"
#include "gfx/renderer/renderer.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/fence.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/queue_family.hpp"
#include "gfx/vulkan/semaphore.hpp"
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
		renderer = nullptr;
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
			.imageExtent = VkExtent2D{(extent.x), (extent.y)},
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

		image_available_semaphores.clear();
		in_flight_fences.clear();
		for (uint32_t i = 0; i < device_ptr->get_image_count(); ++i)
		{
			image_available_semaphores.emplace_back(std::make_shared<Semaphore>(device));
			in_flight_fences.emplace_back(std::make_shared<Fence>(device, true));
		}
	}

	void Swapchain::render()
	{
		if (!renderer)
			return;
		if (!render_internal())
			return;

		create_or_recreate();
		renderer->resize(extent);
		if (render_internal())
			LOG_ERROR("Failed to render frame");
	}

	const Semaphore& Swapchain::get_image_available_semaphore(uint32_t image_index) const
	{
		return *image_available_semaphores[image_index];
	}

	const Fence& Swapchain::get_in_flight_fence(uint32_t image_index) const
	{
		return *in_flight_fences[image_index];
	}

	bool Swapchain::render_internal()
	{
		const auto device_ref = device.lock();
		uint32_t current_frame = device.lock()->get_current_image();

		in_flight_fences[current_frame]->wait();

		uint32_t image_index;
		const VkResult acquire_result = vkAcquireNextImageKHR(device_ref->raw(), ptr, UINT64_MAX,
		                                                      image_available_semaphores[current_frame]->raw(),
		                                                      VK_NULL_HANDLE, &image_index);
		if (acquire_result != VK_SUCCESS)
		{
			if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR || acquire_result == VK_SUBOPTIMAL_KHR)
				return true;
			VK_CHECK(acquire_result, "Failed to acquire next swapchain image");
		}

		renderer->render(image_index, current_frame);

		// Submit to present queue
		const auto render_finished_semaphore = renderer->get_render_finished_semaphore(image_index).raw();
		const VkPresentInfoKHR present_infos{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &render_finished_semaphore,
			.swapchainCount = 1,
			.pSwapchains = &ptr,
			.pImageIndices = &image_index,
			.pResults = nullptr,
		};

		const VkResult submit_result = device_ref->get_queues().get_queue(QueueSpecialization::Present)->present(
			present_infos);
		if (submit_result == VK_ERROR_OUT_OF_DATE_KHR || submit_result == VK_SUBOPTIMAL_KHR)
			return true;
		if (submit_result != VK_SUCCESS)
			VK_CHECK(acquire_result, "Failed to present swapchain image");

		return false;
	}

	void Swapchain::destroy()
	{
		device.lock()->wait();
		image_view = nullptr;

		if (ptr != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(device.lock()->raw(), ptr, nullptr);
			ptr = VK_NULL_HANDLE;
		}
	}

	void Swapchain::set_renderer(const std::shared_ptr<PresentStep>& present_step)
	{
		const auto render_step = present_step->init_for_swapchain(*this);
		renderer = std::make_shared<SwapchainPresentPass>(
			device.lock()->find_or_create_render_pass(render_step->get_infos()), weak_from_this(), render_step);
	}

	std::weak_ptr<ImageView> Swapchain::get_image_view() const
	{
		return image_view;
	}
}
