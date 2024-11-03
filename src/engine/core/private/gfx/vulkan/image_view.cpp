#include "gfx/vulkan/image_view.hpp"

#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/vk_check.hpp"

namespace Engine
{
	ImageView::ImageView(const std::shared_ptr<Image>& in_image) : device(in_image->get_device())
	{
		for (const auto img : in_image->get_resource())
			views.emplace_back(
				std::make_shared<ImageViewResource>(device, img, CreateInfos{.format = in_image->get_params().format}));
	}

	ImageView::ImageView(std::weak_ptr<Device> in_device, std::vector<VkImage> raw_image,
	                     CreateInfos create_infos): device(in_device)
	{
		for (const auto img : raw_image)
			views.emplace_back(std::make_shared<ImageViewResource>(device, img, create_infos));
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
		for (const auto& view : views)
			ptrs.emplace_back(view->ptr);
		return ptrs;
	}

	const VkDescriptorImageInfo& ImageView::get_descriptor_infos_current()
	{
		if (views.size() == 1)
			return views[0]->descriptor_infos;
		return views[device.lock()->get_current_image()]->descriptor_infos;
	}

	ImageViewResource::ImageViewResource(const std::weak_ptr<Device>& device,
	                                     std::shared_ptr<Image::ImageResource> resource,
	                                     ImageView::CreateInfos create_infos) :
		ImageViewResource(device, resource->ptr, create_infos)
	{
		image_resource = resource;
	}

	ImageViewResource::ImageViewResource(const std::weak_ptr<Device>& device, VkImage image,
	                                     ImageView::CreateInfos create_infos) :
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
				.aspectMask = static_cast<VkImageAspectFlags>(is_depth_format(create_infos.format)
					                                              ? VK_IMAGE_ASPECT_DEPTH_BIT
					                                              : VK_IMAGE_ASPECT_COLOR_BIT),
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			}
		};
		VK_CHECK(vkCreateImageView(device.lock()->raw(), &image_view_infos, nullptr, &ptr),
		         "failed to create swapchain image views");

		descriptor_infos = VkDescriptorImageInfo{
			.sampler = VK_NULL_HANDLE,
			.imageView = ptr,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};
	}

	ImageViewResource::~ImageViewResource()
	{
		vkDestroyImageView(device().lock()->raw(), ptr, nullptr);
	}
}
