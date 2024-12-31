#pragma once
#include <memory>
#include <utility>
#include <vector>

#include "gfx/types.hpp"
#include "image.hpp"

namespace Eng::Gfx
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
        uint32_t    mip_levels  = 1;
        uint32_t    layer_count = 1;
        bool        is_storage  = false;
    };

    static std::shared_ptr<ImageView> create(std::string name, const std::shared_ptr<Image>& image)
    {
        return std::shared_ptr<ImageView>(new ImageView(std::move(name), image));
    }

    static std::shared_ptr<ImageView> create(std::string name, std::weak_ptr<Device> device, std::vector<VkImage> raw_image, CreateInfos create_infos)
    {
        return std::shared_ptr<ImageView>(new ImageView(std::move(name), std::move(device), std::move(raw_image), create_infos));
    }

    ImageView(ImageView&&) = delete;
    ImageView(ImageView&)  = delete;
    ~ImageView();

    VkImageView              raw_current() const;
    std::vector<VkImageView> raw() const;

    const VkDescriptorImageInfo& get_descriptor_infos_current() const;

    const std::string& get_name() const
    {
        return name;
    }

    std::shared_ptr<Image> get_base_image() const
    {
        return image;
    }

private:
    class Resource : public DeviceResource
    {
    public:
        Resource(std::string name, const std::weak_ptr<Device>& device, const std::shared_ptr<Image::ImageResource>& resource, CreateInfos create_infos);
        Resource(std::string name, const std::weak_ptr<Device>& device, VkImage image, CreateInfos create_infos);
        ~Resource();
        VkImageView                           ptr = VK_NULL_HANDLE;
        VkDescriptorImageInfo                 descriptor_infos;
        std::shared_ptr<Image::ImageResource> image_resource;
    };

    ImageView(std::string name, const std::shared_ptr<Image>& image);
    ImageView(std::string name, std::weak_ptr<Device> device, std::vector<VkImage> raw_image, CreateInfos create_infos);
    std::vector<std::shared_ptr<Resource>> views;
    std::shared_ptr<Image>                 image;
    std::weak_ptr<Device>                  device;
    std::string                            name;
};
} // namespace Eng::Gfx