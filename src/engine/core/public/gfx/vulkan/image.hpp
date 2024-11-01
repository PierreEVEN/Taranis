#pragma once
#include <memory>
#include <optional>
#include <glm/vec2.hpp>
#include <vulkan/vulkan_core.h>

#include "buffer.hpp"
#include "device.hpp"
#include "vk_mem_alloc.h"
#include "gfx/types.hpp"

namespace Engine
{
	class CommandBuffer;
	class Device;

	enum class EImageType
	{
		Texture_1D,
		Texture_1D_Array,
		Texture_2D,
		Texture_2D_Array,
		Texture_3D,
		Cubemap,
	};

	enum class ETextureTransferCapabilities
	{
		None = 0x00000,
		CopySource = 0x00001,
		CopyDestination = 0x00002,
	};

	enum class ETextureGPUWriteCapabilities
	{
		None,
		Enabled,
	};

	enum class ETextureGPUReadCapabilities
	{
		None,
		Sampling,
	};

	struct ImageParameter
	{
		ColorFormat format = ColorFormat::R8G8B8A8_UNORM;
		EImageType image_type = EImageType::Texture_2D;
		ETextureTransferCapabilities transfer_capabilities = ETextureTransferCapabilities::CopyDestination;
		ETextureGPUWriteCapabilities gpu_write_capabilities = ETextureGPUWriteCapabilities::None;
		ETextureGPUReadCapabilities gpu_read_capabilities = ETextureGPUReadCapabilities::Sampling;
		std::optional<uint32_t> mip_level;
		EBufferType buffer_type = EBufferType::IMMUTABLE;
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t depth = 1;
		bool read_only = true;
	};

	class Image
	{
	public:

		Image(std::weak_ptr<Device> device, ImageParameter params);
		Image(std::weak_ptr<Device> device, ImageParameter params, const BufferData& data);
		~Image();

		std::vector<VkImage> raw() const;
		VkImage raw_current() ;
		std::weak_ptr<Device> get_device() const { return device; }
		glm::uvec2 get_extent() const { return extent; }

		bool resize(glm::uvec2 new_size);
		void set_data(glm::uvec2 new_size, const BufferData& data);

	private:


		class ImageResource : public DeviceResource, public std::enable_shared_from_this<ImageResource>
		{
		public:
			ImageResource(std::weak_ptr<Device> device, ImageParameter params);
			~ImageResource();
			void set_data(const BufferData& data);
			void set_image_layout(CommandBuffer& command_buffer, VkImageLayout new_layout);

			VkImage ptr = VK_NULL_HANDLE;
			VkImageLayout image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
			VmaAllocation allocation = VK_NULL_HANDLE;
			bool outdated = false;
			uint32_t layer_cout = 0, mip_levels = 0;
			bool is_depth = false;
			glm::uvec2 res;
			uint32_t depth = 0;
		};

		glm::uvec2 extent;
		ImageParameter params;
		std::weak_ptr<Device> device;
		BufferData temp_buffer_data;
		std::vector<std::shared_ptr<ImageResource>> images;
	};
}
