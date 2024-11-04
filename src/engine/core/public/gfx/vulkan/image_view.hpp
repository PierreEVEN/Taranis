#pragma once
#include <memory>
#include <vector>

#include "gfx/types.hpp"
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

    ImageView(std::string name, const std::shared_ptr<Image>& image);
    ImageView(std::string name, std::weak_ptr<Device> device, std::vector<VkImage> raw_image, CreateInfos create_infos);
    ImageView(ImageView&&) = delete;
    ImageView(ImageView&)  = delete;
    ~ImageView()           = default;

    VkImageView              raw_current() const;
    std::vector<VkImageView> raw() const;

    const VkDescriptorImageInfo& get_descriptor_infos_current() const;

    const std::string& get_name() const
    {
        return name;
    }

  private:
    std::vector<std::shared_ptr<ImageViewResource>> views;
    std::shared_ptr<Image>                          image;
    std::weak_ptr<Device>                           device;
    std::string                                     name;
};

class ImageViewResource : public DeviceResource
{
  public:
    ImageViewResource(const std::string& name, const std::weak_ptr<Device>& device, const std::shared_ptr<Image::ImageResource>& resource, ImageView::CreateInfos create_infos);
    ImageViewResource(const std::string& name, const std::weak_ptr<Device>& device, VkImage image, ImageView::CreateInfos create_infos);
    ~ImageViewResource();
    VkImageView                           ptr = VK_NULL_HANDLE;
    VkDescriptorImageInfo                 descriptor_infos;
    std::shared_ptr<Image::ImageResource> image_resource;
};
} // namespace Engine
