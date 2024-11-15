#include <utility>

#include "gfx/vulkan/buffer.hpp"

#include "profiler.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/fence.hpp"

namespace Eng::Gfx
{
void BufferData::copy_to(uint8_t* destination) const
{
    assert(ptr);
    memcpy(destination, ptr, stride * element_count);
}

Buffer::Buffer(std::string in_name, std::weak_ptr<Device> in_device, const CreateInfos& create_infos, size_t in_stride, size_t in_element_count)
    : stride(in_stride), element_count(in_element_count), params(create_infos), device(std::move(in_device)), name(std::move(in_name))
{
    switch (params.type)
    {
    case EBufferType::STATIC:
    case EBufferType::IMMUTABLE:
        buffers = {std::make_shared<Resource>(name, device, create_infos, in_stride, in_element_count)};
        break;
    case EBufferType::DYNAMIC:
    case EBufferType::IMMEDIATE:
        for (size_t i = 0; i < device.lock()->get_image_count(); ++i)
            buffers.emplace_back(std::make_shared<Resource>(name + "_#" + std::to_string(i), device, create_infos, in_stride, in_element_count));
        break;
    }
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
        LOG_FATAL("Cannot resize immutable buffer !!")
    case EBufferType::STATIC:
        for (const auto& image : buffers)
            device.lock()->drop_resource(image);
        buffers = {std::make_shared<Resource>(name, device, params, new_stride, new_element_count)};
        break;
    case EBufferType::DYNAMIC:
        for (const auto& buffer : buffers)
            device.lock()->drop_resource(buffer);
        buffers.clear();
        for (size_t i = 0; i < device.lock()->get_image_count(); ++i)
            buffers.emplace_back(std::make_shared<Resource>(name, device, params, new_stride, new_element_count));
        break;
    case EBufferType::IMMEDIATE:
        auto& current_buffer = buffers[device.lock()->get_current_image()];
        device.lock()->drop_resource(current_buffer);
        buffers[device.lock()->get_current_image()] = std::make_shared<Resource>(name, device, params, new_stride, new_element_count);
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
        LOG_FATAL("Cannot resize immutable buffer !!")
    case EBufferType::STATIC:
        device.lock()->wait();
        buffers[0]->set_data(start_index, data);
        break;
    case EBufferType::DYNAMIC:
        if (start_index != 0)
            LOG_FATAL("Cannot update ptr inside a dynamic buffer with offset")
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
    LOG_FATAL("Unhandled case")
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

Buffer::Resource::Resource(const std::string& name, std::weak_ptr<Device> in_device, const CreateInfos& create_infos, size_t in_stride, size_t in_element_count)
    : DeviceResource(std::move(in_device)), stride(in_stride), element_count(in_element_count)
{
    assert(element_count != 0 && stride != 0);

    VkMemoryPropertyFlags required_flags = 0;
    VkBufferUsageFlags    vk_usage       = 0;
    VmaAllocationCreateFlags flags          = 0;
    host_visible                         = false;

    switch (create_infos.usage)
    {
    case EBufferUsage::INDEX_DATA:
        vk_usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        vk_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        break;
    case EBufferUsage::VERTEX_DATA:
        vk_usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        vk_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
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
        host_visible   = true;
        flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        required_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        break;
    }

    if (create_infos.type == EBufferType::IMMEDIATE)
    {
        host_visible = true;
        flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
        required_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }

    if (create_infos.type != EBufferType::IMMUTABLE)
    {
        vk_usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        vk_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    const VkBufferCreateInfo buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = element_count * stride,
        .usage = vk_usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    const VmaAllocationCreateInfo allocInfo = {
        .flags = flags,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = required_flags,
    };

    VK_CHECK(vmaCreateBuffer(device().lock()->get_allocator(), &buffer_create_info, &allocInfo, &ptr, &allocation, nullptr), "failed to create buffer")
    device().lock()->debug_set_object_name(name, ptr);
}

Buffer::Resource::~Resource()
{
    vmaDestroyBuffer(device().lock()->get_allocator(), ptr, allocation);
}

void Buffer::Resource::set_data(size_t start_index, const BufferData& data)
{
    if (host_visible)
    {
        PROFILER_SCOPE(CopyBufferDirect);
        outdated      = false;
        void* dst_ptr = nullptr;
        VK_CHECK(vmaMapMemory(device().lock()->get_allocator(), allocation, &dst_ptr), "failed to map memory")
        data.copy_to(static_cast<uint8_t*>(dst_ptr) + start_index * data.get_stride());

        vmaUnmapMemory(device().lock()->get_allocator(), allocation);
    }
    else
    {
        PROFILER_SCOPE(CopyBufferIndirect);
        auto transfer_buffer = Buffer::create("transfer_buffer", device(), Buffer::CreateInfos{.usage = EBufferUsage::TRANSFER_MEMORY}, data);

        auto command_buffer = CommandBuffer::create("transfer_cmd1", device(), QueueSpecialization::Transfer);

        command_buffer->begin(true);

        const VkBufferCopy region = {
            .srcOffset = 0,
            .dstOffset = start_index,
            . size = data.get_byte_size(),
        };

        vkCmdCopyBuffer(command_buffer->raw(), transfer_buffer->raw_current(), ptr, 1, &region);
        command_buffer->end();

        const auto fence = Fence::create("fence", device());
        command_buffer->submit({}, &*fence);
        fence->wait();
    }
}
} // namespace Eng::Gfx