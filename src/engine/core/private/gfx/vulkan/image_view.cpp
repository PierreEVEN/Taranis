#include "gfx/vulkan/image_view.hpp"

#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/vk_check.hpp"

namespace Engine
{
	ImageView::ImageView(std::shared_ptr<Image> in_image, CreateInfos create_infos) : ImageView(
		in_image->get_device(), in_image->raw(), create_infos)
	{
	}

	ImageView::ImageView(std::weak_ptr<Device> in_device, std::vector<VkImage> raw_image,
	                     CreateInfos create_infos): device(in_device)
	{
		for (const auto image : raw_image)
			views.emplace_back(std::make_shared<ImageViewResource>(device, image, create_infos));
	}

	ImageView::~ImageView()
	{
	}

	VkImageView ImageView::raw_current() const
	{
		if (views.size() == 1)
			return views[0]->ptr;
		return views[device.lock()->get_current_image()]->ptr;
	}

	std::vector<VkImageView> ImageView::raw() const
	{
		std::vector<VkImageView> ptrs;
		for (const auto& views : views)
			ptrs.emplace_back(views->ptr);
		return ptrs;
	}

	ImageViewResource::ImageViewResource(const std::weak_ptr<Device>& device, VkImage image,
	                                     ImageView::CreateInfos create_infos):
		DeviceResource(device)
	{
		VkImageViewCreateInfo image_view_infos{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = static_cast<VkFormat>(create_infos.format),
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
		VK_CHECK(vkCreateImageView(device.lock()->raw(), &image_view_infos, nullptr, &ptr),
		         "failed to create swapchain image views");
	}

	ImageViewResource::~ImageViewResource()
	{
		vkDestroyImageView(device().lock()->raw(), ptr, nullptr);
	}
}
