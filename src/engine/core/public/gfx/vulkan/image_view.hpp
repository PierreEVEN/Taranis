#pragma once
#include <memory>
#include <vector>

#include "image.hpp"

namespace Engine
{
	class ImageViewResource;
	class Device;
	class Image;

	class ImageView
	{
	public:
		struct CreateInfos
		{
			ColorFormat format;
		};

		ImageView(std::shared_ptr<Image> image, CreateInfos create_infos);
		ImageView(std::weak_ptr<Device> device, std::vector<VkImage> raw_image, CreateInfos create_infos);
		~ImageView();

		VkImageView raw_current() const;
		std::vector<VkImageView> raw() const;

	private:
		std::vector<std::shared_ptr<ImageViewResource>> views;
		std::shared_ptr<Image> image;
		std::weak_ptr<Device> device;
	};

	class ImageViewResource : public DeviceResource
	{
	public:
		ImageViewResource(const std::weak_ptr<Device>& device, VkImage image, ImageView::CreateInfos create_infos);
		~ImageViewResource();
		VkImageView ptr = VK_NULL_HANDLE;
	};
}
