#pragma once
#include "device_resource.hpp"

#include <memory>
#include <string>
#include <ankerl/unordered_dense.h>
#include <vulkan/vulkan_core.h>

namespace Eng::Gfx
{
class PipelineLayout;
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
    static std::shared_ptr<DescriptorSet> create(const std::string& name, const std::weak_ptr<Device>& device, const std::shared_ptr<PipelineLayout>& pipeline, bool b_static = false);

    const VkDescriptorSet& raw_current() const;

    void bind_image(const std::string& binding_name, const std::shared_ptr<ImageView>& in_image)
    {
        bind_images(binding_name, {in_image});
    }

    void bind_sampler(const std::string& binding_name, const std::shared_ptr<Sampler>& in_sampler)
    {
        bind_samplers(binding_name, {in_sampler});
    }

    void bind_buffer(const std::string& binding_name, const std::shared_ptr<Buffer>& in_buffer)
    {
        bind_buffers(binding_name, {in_buffer});
    }

    void bind_images(const std::string& binding_name, const std::vector<std::shared_ptr<ImageView>>& in_images);
    void bind_samplers(const std::string& binding_name, const std::vector<std::shared_ptr<Sampler>>& in_samplers);
    void bind_buffers(const std::string& binding_name, const std::vector<std::shared_ptr<Buffer>>& in_buffers);

private:
    class Resource : public DeviceResource
    {
    public:
        Resource(const std::string& name, const std::weak_ptr<Device>& device, const std::weak_ptr<DescriptorSet>& parent, const std::shared_ptr<PipelineLayout>& in_pipeline);
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
        bool                            outdated   = false;
        size_t                          pool_index = 0;
        std::shared_ptr<PipelineLayout> pipeline;
        std::weak_ptr<DescriptorSet>    parent;
        VkDescriptorSet                 ptr = VK_NULL_HANDLE;
    };

    DescriptorSet(const std::string& name, const std::weak_ptr<Device>& device, const std::shared_ptr<PipelineLayout>& pipeline, bool b_static);

    class Descriptor
    {
    public:
        Descriptor()             = default;
        Descriptor(Descriptor&)  = delete;
        Descriptor(Descriptor&&) = delete;

        bool operator==(const Descriptor& other) const
        {
            return get_type_id() == other.get_type_id() && equals(other);
        }

        virtual void get_resources(uint32_t& buffer_count, uint32_t& image_count) = 0;
        virtual void fill(std::vector<VkWriteDescriptorSet>& out_sets, VkDescriptorSet dst_set, uint32_t binding, std::vector<VkDescriptorImageInfo>& image_descs, std::vector<VkDescriptorBufferInfo>& buffer_descs) = 0;
        virtual uint32_t get_type_id() const = 0;

    protected:
        virtual bool equals(const Descriptor& other) const = 0;
    };

    class ImagesDescriptor : public Descriptor
    {
    public:
        ImagesDescriptor(std::vector<std::shared_ptr<ImageView>> in_images) : images(std::move(in_images))
        {
        }

        void get_resources(uint32_t&, uint32_t& image_count) override
        {
            image_count += static_cast<uint32_t>(images.size());
        }

        void fill(std::vector<VkWriteDescriptorSet>& out_sets, VkDescriptorSet dst_set, uint32_t binding, std::vector<VkDescriptorImageInfo>& image_descs, std::vector<VkDescriptorBufferInfo>& buffer_descs) override;

        uint32_t get_type_id() const override
        {
            return 1;
        }

    protected:
        bool equals(const Descriptor& other) const override;

        std::vector<std::shared_ptr<ImageView>> images;
    };

    class SamplerDescriptor : public Descriptor
    {
    public:
        SamplerDescriptor(std::vector<std::shared_ptr<Sampler>> in_samplers) : samplers(std::move(in_samplers))
        {
        }

    public:
        void get_resources(uint32_t&, uint32_t& image_count) override
        {
            image_count += static_cast<uint32_t>(samplers.size());
        }

        void fill(std::vector<VkWriteDescriptorSet>& out_sets, VkDescriptorSet dst_set, uint32_t binding, std::vector<VkDescriptorImageInfo>& image_descs, std::vector<VkDescriptorBufferInfo>& buffer_descs) override;

        uint32_t get_type_id() const override
        {
            return 2;
        }

    protected:
        bool                                  equals(const Descriptor& other) const override;
        std::vector<std::shared_ptr<Sampler>> samplers;
    };

    class BufferDescriptor : public Descriptor
    {
    public:
        BufferDescriptor(std::vector<std::shared_ptr<Buffer>> in_buffer) : buffers(std::move(in_buffer))
        {
        }

        void get_resources(uint32_t& buffer_count, uint32_t&) override
        {
            buffer_count += static_cast<uint32_t>(buffers.size());
        }

        void fill(std::vector<VkWriteDescriptorSet>& out_sets, VkDescriptorSet dst_set, uint32_t binding, std::vector<VkDescriptorImageInfo>& image_descs, std::vector<VkDescriptorBufferInfo>& buffer_descs) override;

        uint32_t get_type_id() const override
        {
            return 3;
        }

    protected:
        bool                                 equals(const Descriptor& other) const override;
        std::vector<std::shared_ptr<Buffer>> buffers;
    };

    bool try_insert(const std::string& binding_name, const std::shared_ptr<Descriptor>& descriptor);

    ankerl::unordered_dense::map<std::string, std::shared_ptr<Descriptor>> write_descriptors;
    ankerl::unordered_dense::map<std::string, uint32_t>                    descriptor_bindings;

    std::vector<std::shared_ptr<Resource>> resources;
    mutable std::mutex                     update_lock;
    std::weak_ptr<Device>                  device;
    bool                                   b_static;
    std::string                            name;
};
} // namespace Eng::Gfx