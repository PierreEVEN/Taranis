#include <utility>

#include "gfx/vulkan/image_view.hpp"

#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/vk_check.hpp"

namespace Eng::Gfx
{
ImageView::ImageView(std::string in_name, const std::shared_ptr<Image>& in_image) : image(in_image), device(in_image->get_device()), name(std::move(in_name))
{
    for (size_t i = 0; i < in_image->get_resources().size(); ++i)
        views.emplace_back(std::make_shared<Resource>(name + "_#" + std::to_string(i), device, in_image->get_resources()[i],
                                                      CreateInfos{.format = in_image->get_params().format, .mip_levels = in_image->get_mips_count(), .layer_count = in_image->get_params().array_size}));
}

ImageView::ImageView(std::string in_name, std::weak_ptr<Device> in_device, std::vector<VkImage> raw_image, CreateInfos create_infos) : device(std::move(in_device)), name(std::move(in_name))
{
    for (size_t i = 0; i < raw_image.size(); ++i)
        views.emplace_back(std::make_shared<Resource>(name + "_#" + std::to_string(i), device, raw_image[i], create_infos));
}

ImageView::~ImageView()
{
    if (views.size() != 1)
    {
        for (size_t i = 0; i < views.size(); ++i)
            device.lock()->drop_resource(views[i], i);
    }
    else
        for (const auto& resource : views)
            device.lock()->drop_resource(resource);
}

VkImageView ImageView::raw_current() const
{
    if (views.size() == 1)
        return views[0]->ptr;
    return views[device.lock()->get_current_image()]->ptr;
}

std::vector<VkImageView> ImageView::raw() const
{
    std::vector<VkImageView> image_view_ptr;
    for (const auto& view : views)
        image_view_ptr.emplace_back(view->ptr);
    return image_view_ptr;
}

const VkDescriptorImageInfo& ImageView::get_descriptor_infos_current() const
{
    if (views.size() == 1)
        return views[0]->descriptor_infos;
    return views[device.lock()->get_current_image()]->descriptor_infos;
}

ImageView::Resource::Resource(std::string in_name, const std::weak_ptr<Device>& device, const std::shared_ptr<Image::ImageResource>& resource, CreateInfos create_infos)
    : Resource(std::move(in_name), device, resource->ptr, create_infos)
{
    image_resource = resource;
}

ImageView::Resource::Resource(std::string in_name, const std::weak_ptr<Device>& device, VkImage image, CreateInfos create_infos) : DeviceResource(std::move(in_name), device)
{
    VkImageViewCreateInfo image_view_infos{.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                           .image = image,
                                           .viewType = VK_IMAGE_VIEW_TYPE_2D,
                                           .format = static_cast<VkFormat>(create_infos.format),
                                           .components =
                                           {
                                               .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                               .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                               .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                               .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                                           },
                                           .subresourceRange = {
                                               .aspectMask = static_cast<VkImageAspectFlags>(is_depth_format(create_infos.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT),
                                               .baseMipLevel = 0,
                                               .levelCount = create_infos.mip_levels,
                                               .baseArrayLayer = 0,
                                               .layerCount = create_infos.layer_count,
                                           }};
    VK_CHECK(vkCreateImageView(device.lock()->raw(), &image_view_infos, nullptr, &ptr), "failed to create swapchain image views")

    descriptor_infos = VkDescriptorImageInfo{
        .sampler = VK_NULL_HANDLE,
        .imageView = ptr,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    device.lock()->debug_set_object_name(name(), ptr);
}

ImageView::Resource::~Resource()
{
    vkDestroyImageView(device().lock()->raw(), ptr, nullptr);
}
} // namespace Eng::Gfx