#include <utility>

#include "gfx/vulkan/buffer.hpp"

namespace Engine
{
	void BufferData::copy_to(void* destination) const
	{
		memcpy(destination, data, stride * element_count);
	}

	Buffer::Buffer(std::weak_ptr<Device> in_device, const CreateInfos& create_infos) : params(create_infos),
		device(std::move(in_device))
	{
		switch (params.type)
		{
		case EBufferType::STATIC:
		case EBufferType::IMMUTABLE:
			buffers = {std::make_shared<BufferResource>(device, create_infos)};
			break;
		case EBufferType::DYNAMIC:
		case EBufferType::IMMEDIATE:
			for (size_t i = 0; i < device.lock()->get_image_count(); ++i)
				buffers.emplace_back(std::make_shared<BufferResource>(device, create_infos));
			break;
		}
	}

	Buffer::Buffer(std::weak_ptr<Device> device, const Buffer::CreateInfos& create_infos,
	               const BufferData& data) : Buffer(std::move(device), create_infos)
	{
		for (const auto& buffer : buffers)
			buffer->set_data(data);
	}

	Buffer::~Buffer()
	{
		for (const auto& buffer : buffers)
			buffer->drop();
	}

	bool Buffer::resize(size_t new_stride, size_t new_element_count)
	{
		if (new_stride == stride && new_element_count == element_count)
			return false;

		switch (params.type)
		{
		case EBufferType::IMMUTABLE:
			LOG_FATAL("Cannot resize immutable buffer !!");
		case EBufferType::STATIC:
			for (const auto& image : buffers)
				image->drop();
			buffers = {std::make_shared<BufferResource>(device, params)};
			break;
		case EBufferType::DYNAMIC:
		case EBufferType::IMMEDIATE:
			for (const auto& image : buffers)
				image->drop();
			buffers.clear();
			for (size_t i = 0; i < device.lock()->get_image_count(); ++i)
				buffers.emplace_back(std::make_shared<BufferResource>(device, params));
			break;
		}
		stride = new_stride;
		element_count = new_element_count;
		return true;
	}

	void Buffer::set_data(const BufferData& data)
	{
		if (data.get_stride() * data.get_element_count() > stride * element_count)
			resize(data.get_stride(), data.get_element_count());


		switch (params.type)
		{
		case EBufferType::IMMUTABLE:
			LOG_FATAL("Cannot resize immutable buffer !!");
		case EBufferType::STATIC:
			device.lock()->wait();
			buffers[0]->set_data(data);
			break;
		case EBufferType::DYNAMIC:
			for (size_t i = 0; i < device.lock()->get_image_count(); ++i)
			{
				if (i == device.lock()->get_current_image())
					buffers[i]->set_data(data);
				else
					buffers[i]->outdated = true;
			}
			if (buffers.size() > 1)
				temp_buffer_data = data.copy();
			break;
		case EBufferType::IMMEDIATE:
			buffers[device.lock()->get_current_image()]->set_data(data);
			break;
		}
	}

	std::vector<VkBuffer> Buffer::raw() const
	{
		std::vector<VkBuffer> ptrs;
		for (const auto& buffer : buffers)
			ptrs.emplace_back(buffer->ptr);
		return ptrs;
	}

	VkBuffer Buffer::raw_current()
	{
		bool all_ready = true;
		switch (params.type)
		{
		case EBufferType::IMMUTABLE:
		case EBufferType::STATIC:
			return buffers[0]->ptr;
		case EBufferType::DYNAMIC:
			if (buffers[device.lock()->get_current_image()]->outdated)
			{
				buffers[device.lock()->get_current_image()]->set_data(temp_buffer_data);
			}
			for (const auto& buffer : buffers)
				if (!buffer->outdated)
					all_ready = false;
			if (all_ready)
				temp_buffer_data = BufferData();
		case EBufferType::IMMEDIATE:
			return buffers[device.lock()->get_current_image()]->ptr;
		}
		LOG_FATAL("Unhandled case");
	}

	BufferResource::BufferResource(std::weak_ptr<Device> in_device, const Buffer::CreateInfos& create_infos):
		DeviceResource(std::move(in_device))
	{
		VkBufferUsageFlags vk_usage = 0;
		VmaMemoryUsage vma_usage = VMA_MEMORY_USAGE_UNKNOWN;

		switch (create_infos.usage)
		{
		case EBufferUsage::INDEX_DATA:
			vk_usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			break;
		case EBufferUsage::VERTEX_DATA:
			vk_usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			break;
		case EBufferUsage::GPU_MEMORY:
			vk_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			break;
		case EBufferUsage::UNIFORM_BUFFER:
			vk_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			break;
		case EBufferUsage::INDIRECT_DRAW_ARGUMENT:
			vk_usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
			break;
		case EBufferUsage::TRANSFER_MEMORY:
			vk_usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			break;
		}

		if (create_infos.type != EBufferType::IMMUTABLE)
		{
			vk_usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			vk_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}

		switch (create_infos.access)
		{
		case EBufferAccess::DEFAULT:
			vma_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
			break;
		case EBufferAccess::GPU_ONLY:
			vma_usage = VMA_MEMORY_USAGE_GPU_ONLY;
			break;
		case EBufferAccess::CPU_TO_GPU:
			vma_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
			break;
		case EBufferAccess::GPU_TO_CPU:
			vma_usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
			break;
		}

		const VkBufferCreateInfo buffer_create_info = {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = NULL,
			.size = create_infos.element_count * create_infos.stride,
			.usage = vk_usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
		};

		const VmaAllocationCreateInfo allocInfo = {
			.flags = NULL,
			.usage = vma_usage,
			.requiredFlags = NULL,
			.preferredFlags = NULL,
			.memoryTypeBits = 0,
			.pool = VK_NULL_HANDLE,
			.pUserData = nullptr,
		};
		VK_CHECK(
			vmaCreateBuffer(device().lock()->get_allocator(), &buffer_create_info, &allocInfo, &ptr, &allocation,
				nullptr), "failed to create buffer");
	}

	BufferResource::~BufferResource()
	{
		vmaDestroyBuffer(device().lock()->get_allocator(), ptr, allocation);
	}

	void BufferResource::set_data(const BufferData& data)
	{
		outdated = false;
		void* dst_ptr = nullptr;
		VK_CHECK(vmaMapMemory(device().lock()->get_allocator(), allocation, &dst_ptr), "failed to map memory");
		data.copy_to(dst_ptr);
		vmaUnmapMemory(device().lock()->get_allocator(), allocation);
	}
}
