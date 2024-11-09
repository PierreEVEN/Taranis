#pragma once
#include <memory>
#include <utility>
#include <vector>

#include "device.hpp"

namespace Eng::Gfx
{
class Device;

enum class EBufferType
{
    IMMUTABLE, // No updates allowed
    STATIC,    // Pretty never updated. Updating ptr would cause some freezes (low memory footprint)
    DYNAMIC,
    // Data is stored internally, then automatically submitted. Can lead to a memory overhead depending on the get size.
    IMMEDIATE, // Data need to be submitted every frames
};

enum class EBufferUsage
{
    INDEX_DATA = 0x00000001,             // used as index get
    VERTEX_DATA = 0x00000002,            // used as vertex get
    GPU_MEMORY = 0x00000003,             // used as storage get
    UNIFORM_BUFFER = 0x00000004,         // used as uniform get
    INDIRECT_DRAW_ARGUMENT = 0x00000005, // used for indirect begin commands
    TRANSFER_MEMORY = 0x00000006,        // used for indirect begin commands
};

enum class EBufferAccess
{
    DEFAULT = 0x00000000,    // Choose best configuration
    GPU_ONLY = 0x00000001,   // Data will be cached on GPU
    CPU_TO_GPU = 0x00000002, // frequent transfer from CPU to GPU
    GPU_TO_CPU = 0x00000003, // frequent transfer from GPU to CPU
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

    BufferData(BufferData&) = delete;

    ~BufferData()
    {
        if (own_data)
            free(ptr);
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
        EBufferUsage  usage;
        EBufferAccess access = EBufferAccess::DEFAULT;
        EBufferType   type   = EBufferType::IMMUTABLE;
    };

    static std::shared_ptr<Buffer> create(std::string name, std::weak_ptr<Device> device, const CreateInfos& create_infos, size_t stride, size_t element_count)
    {
        return std::shared_ptr<Buffer>(new Buffer(std::move(name), std::move(device), create_infos, stride, element_count));
    }

    static std::shared_ptr<Buffer> create(const std::string& name, std::weak_ptr<Device> device, const CreateInfos& create_infos, const BufferData& data)
    {
        auto new_buffer = std::shared_ptr<Buffer>(new Buffer(name, std::move(device), create_infos, data.get_stride(), data.get_element_count()));
        for (const auto& buffer : new_buffer->buffers)
            buffer->set_data(0, data);
        return new_buffer;
    }

    Buffer(Buffer&)  = delete;
    Buffer(Buffer&&) = delete;

    ~Buffer();

    bool resize(size_t stride, size_t element_count);

    void set_data(size_t start_index, const BufferData& data);

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
        void          set_data(size_t start_index, const BufferData& data);
        size_t        stride        = 0;
        size_t        element_count = 0;
        bool          outdated      = false;
        VkBuffer      ptr           = VK_NULL_HANDLE;
        VmaAllocation allocation    = VK_NULL_HANDLE;
    };

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