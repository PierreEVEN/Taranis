#include "gfx/vulkan/image.hpp"

#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/fence.hpp"
#include "gfx/vulkan/vk_check.hpp"

namespace Engine
{
	VkImageUsageFlags vk_usage(const ImageParameter& texture_parameters)
	{
		VkImageUsageFlags usage_flags = 0;
		if (static_cast<int>(texture_parameters.transfer_capabilities) & static_cast<int>(
			ETextureTransferCapabilities::CopySource))
			usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		if (static_cast<int>(texture_parameters.transfer_capabilities) & static_cast<int>(
			ETextureTransferCapabilities::CopyDestination))
			usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		if (texture_parameters.gpu_write_capabilities == ETextureGPUWriteCapabilities::Enabled)
			usage_flags |= is_depth_format(texture_parameters.format)
				               ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
				               : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		if (texture_parameters.gpu_read_capabilities == ETextureGPUReadCapabilities::Sampling)
			usage_flags |= VK_IMAGE_USAGE_SAMPLED_BIT;

		return usage_flags;
	}

	Image::Image(std::weak_ptr<Device> in_device, ImageParameter in_params) : device(std::move(in_device)),
	                                                                          params(in_params)
	{
		switch (params.buffer_type)
		{
		case EBufferType::STATIC:
		case EBufferType::IMMUTABLE:
			images = {std::make_shared<ImageResource>(device, params)};
			break;
		case EBufferType::DYNAMIC:
		case EBufferType::IMMEDIATE:
			for (size_t i = 0; i < device.lock()->get_image_count(); ++i)
				images.emplace_back(std::make_shared<ImageResource>(device, params));
			break;
		}
	}

	Image::Image(std::weak_ptr<Device> device, ImageParameter params, const BufferData& data)
		: Image(device, params)
	{
		for (const auto& image : images)
			image->set_data(data);
	}

	Image::~Image()
	{
		for (const auto& image : images)
		{
			device.lock()->drop_resource(image);
		}
	}

	std::vector<VkImage> Image::raw() const
	{
		std::vector<VkImage> ptrs;
		for (const auto& image : images)
			ptrs.emplace_back(image->ptr);
		return ptrs;
	}

	VkImage Image::raw_current()
	{
		bool all_ready = true;
		switch (params.buffer_type)
		{
		case EBufferType::IMMUTABLE:
		case EBufferType::STATIC:
			return images[0]->ptr;
		case EBufferType::DYNAMIC:
			if (images[device.lock()->get_current_image()]->outdated)
			{
				images[device.lock()->get_current_image()]->set_data(temp_buffer_data);
			}
			for (const auto& image : images)
				if (!image->outdated)
					all_ready = false;
			if (all_ready)
				temp_buffer_data = BufferData();
		case EBufferType::IMMEDIATE:
			return images[device.lock()->get_current_image()]->ptr;
		}
		LOG_FATAL("Unhandled buffer type");
	}

	bool Image::resize(glm::uvec2 new_size)
	{
		if (extent == new_size || new_size != glm::uvec2{0, 0})
			return false;

		extent = new_size;

		switch (params.buffer_type)
		{
		case EBufferType::IMMUTABLE:
			LOG_FATAL("Cannot resize immutable image !!");
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

	void Image::set_data(glm::uvec2 new_size, const BufferData& data)
	{
		resize(new_size);

		switch (params.buffer_type)
		{
		case EBufferType::IMMUTABLE:
			LOG_FATAL("Cannot update immutable image !!");
		case EBufferType::STATIC:
			device.lock()->wait();
			images[0]->set_data(data);
			images = {nullptr};
			break;
		case EBufferType::DYNAMIC:
			for (size_t i = 0; i < device.lock()->get_image_count(); ++i)
			{
				if (i == device.lock()->get_current_image())
					images[i]->set_data(data);
				else
					images[i]->outdated = true;
			}
			if (images.size() > 1)
				temp_buffer_data = data.copy();
			break;
		case EBufferType::IMMEDIATE:
			images[device.lock()->get_current_image()]->set_data(data);
			break;
		}
	}

	Image::ImageResource::ImageResource(std::weak_ptr<Device> in_device, ImageParameter params): DeviceResource(
		std::move(in_device))
	{
		layer_cout = params.image_type == EImageType::Cubemap ? 6u : 1u;
		mip_levels = params.mip_level ? *params.mip_level : 1;
		is_depth = is_depth_format(params.format);
		depth = params.depth;
		res = {params.width, params.height};

		VkImageCreateInfo image_create_infos{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.format = static_cast<VkFormat>(params.format),
			.mipLevels = params.mip_level ? params.mip_level.value() : 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = vk_usage(params),
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};
		switch (params.image_type)
		{
		case EImageType::Texture_1D:
			image_create_infos.imageType = VK_IMAGE_TYPE_1D;
			image_create_infos.extent = {
				.width = params.width,
				.height = 1,
				.depth = 1,
			};
			image_create_infos.arrayLayers = 1;
			break;
		case EImageType::Texture_1D_Array:
			image_create_infos.imageType = VK_IMAGE_TYPE_1D;
			image_create_infos.extent = {
				.width = params.width,
				.height = 1,
				.depth = 1,
			};
			image_create_infos.arrayLayers = params.depth;
			break;
		case EImageType::Texture_2D:
			image_create_infos.imageType = VK_IMAGE_TYPE_2D;
			image_create_infos.extent = {
				.width = params.width,
				.height = params.height,
				.depth = 1,
			};
			image_create_infos.arrayLayers = 1;
			break;
		case EImageType::Texture_2D_Array:
			image_create_infos.imageType = VK_IMAGE_TYPE_2D;
			image_create_infos.extent = {
				.width = params.width,
				.height = params.height,
				.depth = 1,
			};
			image_create_infos.arrayLayers = params.depth;
			break;
		case EImageType::Texture_3D:
			image_create_infos.imageType = VK_IMAGE_TYPE_3D;
			image_create_infos.extent = {
				.width = params.width,
				.height = params.height,
				.depth = params.depth,
			};
			image_create_infos.arrayLayers = 1;
			break;
		case EImageType::Cubemap:
			image_create_infos.imageType = VK_IMAGE_TYPE_2D;
			image_create_infos.extent = {
				.width = params.width,
				.height = params.height,
				.depth = 1,
			};
			image_create_infos.arrayLayers = 6;
			break;
		}
		const VmaAllocationCreateInfo vma_allocation{
			.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		};
		VmaAllocationInfo infos;
		VK_CHECK(
			vmaCreateImage(device().lock()->get_allocator(), &image_create_infos, &vma_allocation, &ptr, &allocation, &
				infos), "failed to create image");
	}

	Image::ImageResource::~ImageResource()
	{
		vmaDestroyImage(device().lock()->get_allocator(), ptr, allocation);
	}

	void Image::ImageResource::set_data(const BufferData& data)
	{
		Buffer transfer_buffer(device(), Buffer::CreateInfos{.usage = EBufferUsage::TRANSFER_MEMORY}, data);

		std::unique_ptr<CommandBuffer> command_buffer = std::make_unique<CommandBuffer>(
			device(), QueueSpecialization::Transfer);

		command_buffer->begin(true);

		set_image_layout(*command_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		const VkBufferImageCopy region = {
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource =
			{
				.aspectMask = is_depth
					              ? static_cast<VkImageAspectFlags>(VK_IMAGE_ASPECT_DEPTH_BIT)
					              : static_cast<VkImageAspectFlags>(VK_IMAGE_ASPECT_COLOR_BIT),
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.imageOffset = {0, 0, 0},
			.imageExtent = {res.x, res.y, depth},
		};

		vkCmdCopyBufferToImage(command_buffer->raw(), transfer_buffer.raw_current(), ptr,
		                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		                       1, &region);


		command_buffer->end();

		const Fence fence(device());
		command_buffer->submit({}, &fence);
		fence.wait();

		command_buffer = std::make_unique<CommandBuffer>(device(), QueueSpecialization::Graphic);
		command_buffer->begin(true);
		set_image_layout(*command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		command_buffer->end();
		command_buffer->submit({}, &fence);
		fence.wait();
	}

	void Image::ImageResource::set_image_layout(CommandBuffer& command_buffer, VkImageLayout new_layout)
	{
		VkImageMemoryBarrier barrier = VkImageMemoryBarrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.oldLayout = image_layout,
			.newLayout = new_layout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = ptr,
			.subresourceRange =
			VkImageSubresourceRange{
				.aspectMask = is_depth
					              ? static_cast<VkImageAspectFlags>(VK_IMAGE_ASPECT_DEPTH_BIT)
					              : static_cast<VkImageAspectFlags>(VK_IMAGE_ASPECT_COLOR_BIT),
				.baseMipLevel = 0,
				.levelCount = mip_levels,
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

			source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout ==
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
		{
			LOG_FATAL("Unsupported layout transition : from {} to {}", static_cast<uint32_t>(image_layout),
			          static_cast<uint32_t>(new_layout));
		}

		image_layout = new_layout;
		vkCmdPipelineBarrier(command_buffer.raw(), source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1,
		                     &barrier);
	}
}
