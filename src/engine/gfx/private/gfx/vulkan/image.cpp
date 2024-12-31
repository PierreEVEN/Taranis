#include "gfx/vulkan/image.hpp"

#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/fence.hpp"
#include "gfx/vulkan/vk_check.hpp"
#include "gfx/vulkan/vk_wrap.hpp"

#include <vk_mem_alloc.h>

namespace Eng::Gfx
{
VkImageUsageFlags vk_usage(const ImageParameter& texture_parameters)
{
    VkImageUsageFlags usage_flags = 0;
    if (static_cast<int>(texture_parameters.transfer_capabilities) & static_cast<int>(ETextureTransferCapabilities::CopySource) || texture_parameters.generate_mips.does_generates())
        usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (static_cast<int>(texture_parameters.transfer_capabilities) & static_cast<int>(ETextureTransferCapabilities::CopyDestination))
        usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (texture_parameters.is_attachment)
        usage_flags |= is_depth_format(texture_parameters.format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (texture_parameters.can_sample)
        usage_flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (texture_parameters.is_storage)
        usage_flags |= VK_IMAGE_USAGE_STORAGE_BIT;

    return usage_flags;
}

uint32_t Image::get_mips_count() const
{
    return params.generate_mips.does_generates() ? params.generate_mips.desired_mip_count() == 0 ? static_cast<uint32_t>(std::floor(log2(std::max(params.width, params.height)))) + 1 : 0 : provided_mips;
}

Image::Image(std::string in_name, std::weak_ptr<Device> in_device, const ImageParameter& in_params, uint32_t base_mips) : params(in_params), device(std::move(in_device)), name(std::move(in_name))
{
    provided_mips = base_mips;
    switch (params.buffer_type)
    {
    case EBufferType::STATIC:
    case EBufferType::IMMUTABLE:
        images = {std::make_shared<ImageResource>(name, device, params, get_mips_count())};
        break;
    case EBufferType::DYNAMIC:
    case EBufferType::IMMEDIATE:
        for (size_t i = 0; i < device.lock()->get_image_count(); ++i)
            images.emplace_back(std::make_shared<ImageResource>(name + "_#" + std::to_string(i), device, params, get_mips_count()));
        break;
    }
}

Image::~Image()
{
    for (const auto& image : images)
        device.lock()->drop_resource(image);
}

std::vector<VkImage> Image::raw() const
{
    std::vector<VkImage> image_ptr;
    for (const auto& image : images)
        image_ptr.emplace_back(image->ptr);
    return image_ptr;
}

VkImage Image::raw_current()
{
    switch (params.buffer_type)
    {
    case EBufferType::IMMUTABLE:
    case EBufferType::STATIC:
        return images[0]->ptr;
    case EBufferType::DYNAMIC:
    {
        if (images[device.lock()->get_current_image()]->outdated)
            images[device.lock()->get_current_image()]->set_data(temp_buffer_data);
        bool all_ready = true;
        for (const auto& image : images)
            if (!image->outdated)
                all_ready = false;
        if (all_ready)
            temp_buffer_data.clear();
        break;
    }
    case EBufferType::IMMEDIATE:
        return images[device.lock()->get_current_image()]->ptr;
    }
    LOG_FATAL("Unhandled buffer type")
}

bool Image::resize(glm::uvec2 new_size)
{
    if (extent == new_size || new_size != glm::uvec2{0, 0})
        return false;

    extent = new_size;

    switch (params.buffer_type)
    {
    case EBufferType::IMMUTABLE:
        LOG_FATAL("Cannot resize immutable image !!")
    case EBufferType::STATIC:
        for (const auto& image : images)
            device.lock()->drop_resource(image);
        images = {nullptr};
        break;
    case EBufferType::DYNAMIC:
    case EBufferType::IMMEDIATE:
        for (const auto& image : images)
            device.lock()->drop_resource(image);
        images.clear();
        images.resize(device.lock()->get_image_count(), nullptr);
        break;
    }
    return true;
}

void Image::set_data(glm::uvec2 new_size, const std::vector<BufferData>& mips)
{
    resize(new_size);
    assert(mips.size() == provided_mips || params.generate_mips.does_generates());
    switch (params.buffer_type)
    {
    case EBufferType::IMMUTABLE:
        LOG_FATAL("Cannot update immutable image !!")
    case EBufferType::STATIC:
        device.lock()->wait();
        images[0]->set_data(mips);
        images = {nullptr};
        break;
    case EBufferType::DYNAMIC:
        for (size_t i = 0; i < device.lock()->get_image_count(); ++i)
        {
            if (i == device.lock()->get_current_image())
                images[i]->set_data(mips);
            else
                images[i]->outdated = true;
        }
        if (images.size() > 1)
        {
            temp_buffer_data.clear();
            for (const auto& buffer : temp_buffer_data)
                temp_buffer_data.emplace_back(buffer.copy());
        }
        break;
    case EBufferType::IMMEDIATE:
        images[device.lock()->get_current_image()]->set_data(mips);
        break;
    }
}

Image::ImageResource::ImageResource(std::string in_name, std::weak_ptr<Device> in_device, ImageParameter params, uint32_t in_mip_count)
    : DeviceResource(std::move(in_name), std::move(in_device)), format(static_cast<VkFormat>(params.format))
{
    layer_cout = params.image_type == EImageType::Cubemap ? 6u : 1u;
    is_depth   = is_depth_format(params.format);
    depth      = params.depth;
    res        = {params.width, params.height};

    mip_count = in_mip_count;

    VkImageType image_type = VK_IMAGE_TYPE_2D;
    if (params.height > 1)
        image_type = VK_IMAGE_TYPE_2D;
    if (params.depth > 1)
        image_type = VK_IMAGE_TYPE_3D;

    VkImageCreateInfo image_create_infos{
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = image_type,
        .format        = format,
        .extent        = VkExtent3D{params.width, params.height, params.depth},
        .mipLevels     = mip_count,
        .arrayLayers   = params.array_size,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = vk_usage(params),
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    constexpr VmaAllocationCreateInfo vma_allocation{
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
    };
    VmaAllocationInfo infos;
    VmaAllocation     vma_alloc;
    VK_CHECK(vmaCreateImage(device().lock()->get_allocator().allocator, &image_create_infos, &vma_allocation, &ptr, &vma_alloc, &infos), "failed to create image")
    allocation = std::make_unique<VmaAllocationWrap>(vma_alloc);
    device().lock()->debug_set_object_name(name(), ptr);

    if (params.is_storage)
    {
        auto command_buffer = CommandBuffer::create(name() + "_transfer_cmd1", device(), QueueSpecialization::Graphic);
        command_buffer->begin(true);
        set_image_layout(*command_buffer, VK_IMAGE_LAYOUT_GENERAL);
        command_buffer->end();
        const auto fence = Fence::create(name() + "_fence", device());
        command_buffer->submit({}, &*fence);
        fence->wait();
    }
}

Image::ImageResource::~ImageResource()
{
    vmaDestroyImage(device().lock()->get_allocator().allocator, ptr, allocation->allocation);
}

void Image::ImageResource::set_data(const std::vector<BufferData>& mips)
{
    std::vector<std::shared_ptr<Buffer>> transfer_buffers;

    assert(mip_count == mips.size() || generate_mips.does_generates());

    for (const auto& mip : mips)
        transfer_buffers.emplace_back(Buffer::create(name() + "_transfer_buffer", device(), Buffer::CreateInfos{.usage = EBufferUsage::TRANSFER_MEMORY}, mip));

    auto command_buffer = CommandBuffer::create(name() + "_transfer_cmd1", device(), QueueSpecialization::Transfer);

    command_buffer->begin(true);

    set_image_layout(*command_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    uint32_t mipWidth  = res.x;
    uint32_t mipHeight = res.y;
    for (uint32_t mip = 0; mip < transfer_buffers.size(); ++mip)
    {
        const VkBufferImageCopy region = {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource =
            {
                .aspectMask = is_depth ? static_cast<VkImageAspectFlags>(VK_IMAGE_ASPECT_DEPTH_BIT) : static_cast<VkImageAspectFlags>(VK_IMAGE_ASPECT_COLOR_BIT),
                .mipLevel = mip,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .imageOffset = {0, 0, 0},
            .imageExtent = {mipWidth, mipHeight, depth},
        };

        vkCmdCopyBufferToImage(command_buffer->raw(), transfer_buffers[mip]->raw_current(), ptr, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        if (mipWidth > 1)
            mipWidth /= 2;
        if (mipHeight > 1)
            mipHeight /= 2;
    }
    command_buffer->end();

    const auto fence = Fence::create(name() + "_fence", device());
    command_buffer->submit({}, &*fence);
    fence->wait();

    command_buffer = nullptr;
    command_buffer = CommandBuffer::create(name() + "_transfer_cmd2", device(), QueueSpecialization::Graphic);
    command_buffer->begin(true);
    if (generate_mips.does_generates())
        generate_mipmaps(mip_count, *command_buffer);
    set_image_layout(*command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    command_buffer->end();
    command_buffer->submit({}, &*fence);
    fence->wait();
}

void Image::ImageResource::set_image_layout(const CommandBuffer& command_buffer, VkImageLayout new_layout)
{
    if (image_layout == new_layout)
        return;
    VkImageMemoryBarrier barrier = VkImageMemoryBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = image_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = ptr,
        .subresourceRange =
        VkImageSubresourceRange{
            .aspectMask = is_depth ? static_cast<VkImageAspectFlags>(VK_IMAGE_ASPECT_DEPTH_BIT) : static_cast<VkImageAspectFlags>(VK_IMAGE_ASPECT_COLOR_BIT),
            .baseMipLevel = 0,
            .levelCount = mip_count,
            .baseArrayLayer = 0,
            .layerCount = layer_cout,
        },
    };
    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    if (image_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (image_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (image_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_GENERAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (image_layout == VK_IMAGE_LAYOUT_GENERAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        LOG_FATAL("Unsupported layout transition : from {} to {}", static_cast<uint32_t>(image_layout), static_cast<uint32_t>(new_layout))
    }

    image_layout = new_layout;
    vkCmdPipelineBarrier(command_buffer.raw(), source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void Image::ImageResource::generate_mipmaps(uint32_t mipLevels, const CommandBuffer& command_buffer) const
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(device().lock()->get_physical_device().raw(), format, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        LOG_FATAL("This texture image format doesn't support image blitting");

    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image                           = ptr;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.subresourceRange.levelCount     = 1;

    int32_t mipWidth  = res.x;
    int32_t mipHeight = res.y;

    for (uint32_t i = 1; i < mipLevels; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer.raw(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0]                 = {0, 0, 0};
        blit.srcOffsets[1]                 = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel       = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount     = 1;
        blit.dstOffsets[0]                 = {0, 0, 0};
        blit.dstOffsets[1]                 = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel       = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount     = 1;

        vkCmdBlitImage(command_buffer.raw(), ptr, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ptr, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer.raw(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1)
            mipWidth /= 2;
        if (mipHeight > 1)
            mipHeight /= 2;
    }
}
} // namespace Eng::Gfx