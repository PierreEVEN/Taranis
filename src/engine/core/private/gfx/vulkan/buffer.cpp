#include <utility>

#include "gfx/vulkan/buffer.hpp"

namespace Engine
{
	void BufferData::copy_to(uint8_t* destination) const
	{
		memcpy(destination, ptr, stride * element_count);
	}

	Buffer::Buffer(std::weak_ptr<Device> in_device, const CreateInfos& create_infos, size_t in_stride,
	               size_t in_element_count) : stride(in_stride), element_count(in_element_count), params(create_infos),
	                                          device(std::move(in_device))
	{
		switch (params.type)
		{
		case EBufferType::STATIC:
		case EBufferType::IMMUTABLE:
			buffers = {std::make_shared<BufferResource>(device, create_infos, in_stride, in_element_count)};
			break;
		case EBufferType::DYNAMIC:
		case EBufferType::IMMEDIATE:
			for (size_t i = 0; i < device.lock()->get_image_count(); ++i)
				buffers.emplace_back(
					std::make_shared<BufferResource>(device, create_infos, in_stride, in_element_count));
			break;
		}
	}

	Buffer::Buffer(std::weak_ptr<Device> device, const CreateInfos& create_infos,
	               const BufferData& data) : Buffer(std::move(device), create_infos, data.get_stride(),
	                                                data.get_element_count())
	{
		for (const auto& buffer : buffers)
			buffer->set_data(0, data);
	}

	Buffer::~Buffer()
	{
		for (const auto& buffer : buffers)
			device.lock()->drop_resource(buffer);
	}

	bool Buffer::resize(size_t new_stride, size_t new_element_count)
	{
		if (new_stride == get_stride() && new_element_count == get_element_count())
			return false;
		switch (params.type)
		{
		case EBufferType::IMMUTABLE:
			LOG_FATAL("Cannot resize immutable buffer !!");
		case EBufferType::STATIC:
			for (const auto& image : buffers)
				device.lock()->drop_resource(image);
			buffers = {std::make_shared<BufferResource>(device, params, new_stride, new_element_count)};
			break;
		case EBufferType::DYNAMIC:
			for (const auto& buffer : buffers)
				device.lock()->drop_resource(buffer);
			buffers.clear();
			for (size_t i = 0; i < device.lock()->get_image_count(); ++i)
				buffers.emplace_back(std::make_shared<BufferResource>(device, params, new_stride, new_element_count));
			break;
		case EBufferType::IMMEDIATE:
			auto& current_buffer = buffers[device.lock()->get_current_image()];
			device.lock()->drop_resource(current_buffer);
			buffers[device.lock()->get_current_image()] = std::make_shared<BufferResource>(
				device, params, new_stride, new_element_count);
		}
		return true;
	}

	void Buffer::set_data(size_t start_index, const BufferData& data)
	{
		if (data.get_stride() * (start_index + data.get_element_count()) > get_stride() * get_element_count())
			resize(data.get_stride(), data.get_element_count() + start_index);

		switch (params.type)
		{
		case EBufferType::IMMUTABLE:
			LOG_FATAL("Cannot resize immutable buffer !!");
		case EBufferType::STATIC:
			device.lock()->wait();
			buffers[0]->set_data(start_index, data);
			break;
		case EBufferType::DYNAMIC:
			if (start_index != 0)
				LOG_FATAL("Cannot update ptr inside a dynamic buffer with offset");
			for (size_t i = 0; i < device.lock()->get_image_count(); ++i)
			{
				if (i == device.lock()->get_current_image())
					buffers[i]->set_data(0, data);
				else
					buffers[i]->outdated = true;
			}
			if (buffers.size() > 1)
				temp_buffer_data = data.copy();
			break;
		case EBufferType::IMMEDIATE:
			LOG_WARNING("UPDATE BUFFER AAA : {}-{}", start_index, data.get_element_count());
			buffers[device.lock()->get_current_image()]->set_data(start_index, data);
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
				buffers[device.lock()->get_current_image()]->set_data(0, temp_buffer_data);
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

	size_t Buffer::get_element_count() const
	{
		if (buffers.size() != 1)
		{
			return buffers[device.lock()->get_current_image()]->element_count;
		}
		return buffers[0]->element_count;
	}

	size_t Buffer::get_stride() const
	{
		if (buffers.size() != 1)
		{
			return buffers[device.lock()->get_current_image()]->stride;
		}
		return buffers[0]->stride;
	}

	BufferResource::BufferResource(std::weak_ptr<Device> in_device, const Buffer::CreateInfos& create_infos,
	                               size_t in_stride, size_t in_element_count): DeviceResource(std::move(in_device)),
	                                                                           stride(in_stride),
	                                                                           element_count(in_element_count)
	{
		assert(element_count != 0 && stride != 0);

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
			vk_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
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
			.size = element_count * stride,
			.usage = vk_usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};

		const VmaAllocationCreateInfo allocInfo = {
			.usage = vma_usage,
		};
		VK_CHECK(
			vmaCreateBuffer(device().lock()->get_allocator(), &buffer_create_info, &allocInfo, &ptr, &allocation,
				nullptr), "failed to create buffer");
	}

	BufferResource::~BufferResource()
	{
		vmaDestroyBuffer(device().lock()->get_allocator(), ptr, allocation);
	}

	void BufferResource::set_data(size_t start_index, const BufferData& data)
	{
		LOG_WARNING("UPDATE BUFFER BBB : {}-{}", start_index, data.get_element_count());
		outdated = false;
		void* dst_ptr = nullptr;
		VK_CHECK(vmaMapMemory(device().lock()->get_allocator(), allocation, &dst_ptr), "failed to map memory");
		data.copy_to(static_cast<uint8_t*>(dst_ptr) + start_index * data.get_stride());
		vmaUnmapMemory(device().lock()->get_allocator(), allocation);
	}
}
