#pragma once
#include <memory>
#include <utility>
#include <vector>

#include "device.hpp"

struct VmaAllocationWrap;

namespace Eng::Gfx
{
class Device;

enum class EBufferType
{
    IMMUTABLE, // No updates allowed
    STATIC,    // Pretty never updated. Updating ptr would cause some freezes (low memory footprint)
    DYNAMIC,
    // Data is stored internally, then automatically submitted. Can lead to a memory overhead depending on the current_thread size.
    IMMEDIATE, // Data need to be submitted every frames
};

enum class EBufferUsage
{
    INDEX_DATA = 0x00000001,             // used as index current_thread
    VERTEX_DATA = 0x00000002,            // used as vertex current_thread
    GPU_MEMORY = 0x00000003,             // used as storage current_thread
    UNIFORM_BUFFER = 0x00000004,         // used as uniform current_thread
    INDIRECT_DRAW_ARGUMENT = 0x00000005, // used for indirect begin commands
    TRANSFER_MEMORY = 0x00000006,        // host local memory used to transfer data to device memory
};

class BufferData
{
public:
    BufferData() : ptr(nullptr), element_count(0), stride(0)
    {
    }

    template <typename T> BufferData(const T& object) : BufferData(&object, sizeof(T), 1)
    {
    }

    BufferData(const void* in_data, size_t in_stride, size_t in_element_count) : ptr(const_cast<void*>(in_data)), element_count(in_element_count), stride(in_stride)
    {
    }

    BufferData(const BufferData& other)
    {
        own_data = other.own_data;
        if (own_data)
        {
            ptr = std::malloc(other.get_byte_size());
            std::memcpy(ptr, other.ptr, other.get_byte_size());
        }
        else
            ptr = other.ptr;
        element_count       = other.element_count;
        stride              = other.stride;
    }

    BufferData(BufferData&& other) noexcept
    {
        own_data            = other.own_data;
        ptr                 = other.ptr;
        element_count       = other.element_count;
        stride              = other.stride;
        other.ptr           = nullptr;
        other.element_count = 0;
        other.stride        = 0;
        other.own_data      = false;
    }

    ~BufferData()
    {
        if (own_data)
            free(ptr);
    }

    BufferData& operator=(BufferData&& other) noexcept
    {
        own_data            = other.own_data;
        ptr                 = other.ptr;
        element_count       = other.element_count;
        stride              = other.stride;
        other.ptr           = nullptr;
        other.element_count = 0;
        other.stride        = 0;
        other.own_data      = false;
        return *this;
    }

    std::shared_ptr<BufferData> copy() const
    {
        void* new_data = malloc(stride * element_count);
        memcpy(new_data, ptr, stride * element_count);
        auto buffer      = std::make_shared<BufferData>(new_data, stride, element_count);
        buffer->own_data = true;
        return buffer;
    }

    size_t get_stride() const
    {
        return stride;
    }

    size_t get_byte_size() const
    {
        return stride * element_count;
    }

    size_t get_element_count() const
    {
        return element_count;
    }

    void copy_to(uint8_t* destination) const;

    const void* data() const
    {
        return ptr;
    }

private:
    bool   own_data      = false;
    void*  ptr           = nullptr;
    size_t element_count = 0;
    size_t stride        = 0;
};

class Buffer
{
public:
    struct CreateInfos
    {
        EBufferUsage usage;
        EBufferType  type = EBufferType::IMMUTABLE;
    };

    static std::shared_ptr<Buffer> create(std::string name, std::weak_ptr<Device> device, const CreateInfos& create_infos, size_t stride, size_t element_count)
    {
        return std::shared_ptr<Buffer>(new Buffer(std::move(name), std::move(device), create_infos, stride, element_count));
    }

    static std::shared_ptr<Buffer> create(const std::string& name, std::weak_ptr<Device> device, const CreateInfos& create_infos, const BufferData& data)
    {
        auto new_buffer = std::shared_ptr<Buffer>(new Buffer(name, std::move(device), create_infos, data.get_stride(), data.get_element_count()));
        for (const auto& buffer : new_buffer->buffers)
            buffer->set_data_and_wait(0, data);
        return new_buffer;
    }

    Buffer(Buffer&)  = delete;
    Buffer(Buffer&&) = delete;

    const VkDescriptorBufferInfo& get_descriptor_infos_current() const;

    ~Buffer();

    bool resize(size_t stride, size_t element_count);

    void set_data_and_wait(size_t start_index, const BufferData& data);
    void set_data(size_t start_index, const BufferData& data);
    void wait_data_upload() const;

    std::vector<VkBuffer> raw() const;
    VkBuffer              raw_current();

    size_t get_element_count() const;
    size_t get_stride() const;

    size_t get_byte_size() const
    {
        return get_stride() * get_element_count();
    }

    class Resource : public DeviceResource
    {
    public:
        Resource(const std::string& name, std::weak_ptr<Device> device, const Buffer::CreateInfos& create_infos, size_t stride, size_t element_count);
        ~Resource();
        void set_data_and_wait(size_t start_index, const BufferData& data);

        void set_data(size_t start_index, const BufferData& data);
        void wait_data_upload();

        size_t                             stride        = 0;
        size_t                             element_count = 0;
        bool                               outdated      = false;
        bool                               host_visible  = false;
        VkBuffer                           ptr           = VK_NULL_HANDLE;
        std::unique_ptr<VmaAllocationWrap> allocation;
        VkDescriptorBufferInfo             descriptor_data;
        std::string                        name;
        std::shared_ptr<CommandBuffer>     data_update_cmd;
        std::shared_ptr<Fence>             data_update_fence;
        std::shared_ptr<Buffer>            transfer_buffer;
    };

    const std::string& get_name() const
    {
        return name;
    }

private:
    Buffer(std::string name, std::weak_ptr<Device> device, const CreateInfos& create_infos, size_t stride, size_t element_count);
    size_t                                 stride        = 0;
    size_t                                 element_count = 0;
    CreateInfos                            params;
    BufferData                             temp_buffer_data;
    std::vector<std::shared_ptr<Resource>> buffers;
    std::weak_ptr<Device>                  device;
    std::string                            name;
};
} // namespace Eng::Gfx