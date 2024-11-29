#pragma once
#include "device.hpp"
#include <memory>
#include <string>
#include <ankerl/unordered_dense.h>
#include <vulkan/vulkan_core.h>

namespace Eng::Gfx
{
class Buffer;
class ImageView;
class Pipeline;
class Sampler;
class Device;

class DescriptorSet : public std::enable_shared_from_this<DescriptorSet>
{
    friend class Resource;

public:
    DescriptorSet(DescriptorSet&)  = delete;
    DescriptorSet(DescriptorSet&&) = delete;
    ~DescriptorSet();
    static std::shared_ptr<DescriptorSet> create(const std::string& name, const std::weak_ptr<Device>& device, const std::shared_ptr<Pipeline>& pipeline, bool b_static = true);

    const VkDescriptorSet& raw_current() const;

    void       bind_image(const std::string& binding_name, const std::shared_ptr<ImageView>& in_image);
    void       bind_sampler(const std::string& binding_name, const std::shared_ptr<Sampler>& in_sampler);
    void       bind_buffer(const std::string& binding_name, const std::shared_ptr<Buffer>& in_buffer);
    std::mutex test_mtx;

private:
    class Resource : public DeviceResource
    {
    public:
        Resource(const std::string& name, const std::weak_ptr<Device>& device, const std::weak_ptr<DescriptorSet>& parent, const std::shared_ptr<Pipeline>& in_pipeline);
        Resource(Resource&)  = delete;
        Resource(Resource&&) = delete;
        ~Resource();

        void mark_as_dirty()
        {
            outdated = true;
        }

        void update();

        const VkDescriptorSet& raw() const
        {
            return ptr;
        }

    private:
        bool                         outdated   = false;
        size_t                       pool_index = 0;
        std::shared_ptr<Pipeline>    pipeline;
        std::weak_ptr<DescriptorSet> parent;
        VkDescriptorSet              ptr = VK_NULL_HANDLE;
    };

    DescriptorSet(const std::weak_ptr<Device>& device, const std::shared_ptr<Pipeline>& pipeline, bool b_static);

    class Descriptor
    {
    public:
        bool operator==(const Descriptor& other) const
        {
            return raw_ptr() == other.raw_ptr();
        }

        Descriptor()             = default;
        Descriptor(Descriptor&)  = delete;
        Descriptor(Descriptor&&)                     = delete;

        virtual void*                raw_ptr() const = 0;
        virtual VkWriteDescriptorSet get() = 0;
    };

    class ImageDescriptor : public Descriptor
    {
    public:
        ImageDescriptor(std::shared_ptr<ImageView> in_image) : image(std::move(in_image))
        {
        }

        VkWriteDescriptorSet       get() override;
        void*                      raw_ptr() const override;
        std::shared_ptr<ImageView> image;
    };

    class SamplerDescriptor : public Descriptor
    {
    public:
        SamplerDescriptor(std::shared_ptr<Sampler> in_sampler) : sampler(std::move(in_sampler))
        {
        }

        VkWriteDescriptorSet     get() override;
        void*                    raw_ptr() const override;
        std::shared_ptr<Sampler> sampler;
    };

    class BufferDescriptor : public Descriptor
    {
    public:
        BufferDescriptor(std::shared_ptr<Buffer> in_buffer) : buffer(std::move(in_buffer))
        {
        }

        VkWriteDescriptorSet    get() override;
        void*                   raw_ptr() const override;
        std::shared_ptr<Buffer> buffer;
    };

    ankerl::unordered_dense::map<std::string, std::shared_ptr<Descriptor>> write_descriptors;
    ankerl::unordered_dense::map<std::string, uint32_t>                    descriptor_bindings;

    std::vector<std::shared_ptr<Resource>> resources;
    mutable std::mutex                     update_lock;
    std::weak_ptr<Device>                  device;
    bool                                   b_static;
};
} // namespace Eng::Gfx