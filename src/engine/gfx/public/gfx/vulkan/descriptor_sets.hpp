#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan_core.h>
#include "device.hpp"

namespace Engine::Gfx
{
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

    void bind_image(const std::string& binding_name, const std::shared_ptr<ImageView>& in_image);
    void bind_sampler(const std::string& binding_name, const std::shared_ptr<Sampler>& in_sampler);

  private:
    class Resource : public DeviceResource
    {
        friend class DescriptorSet;

      public:
        Resource(const std::string& name, const std::weak_ptr<Device>& device, const std::weak_ptr<DescriptorSet>& parent, const std::shared_ptr<Pipeline>& in_pipeline);
        Resource(Resource&)  = delete;
        Resource(Resource&&) = delete;
        ~Resource();

        void update();

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
        Descriptor()                       = default;
        Descriptor(Descriptor&)            = delete;
        Descriptor(Descriptor&&)           = delete;
        virtual VkWriteDescriptorSet get() = 0;
    };

    class ImageDescriptor : public Descriptor
    {
      public:
        ImageDescriptor(std::shared_ptr<ImageView> in_image) : image(std::move(in_image))
        {
        }

        VkWriteDescriptorSet       get() override;
        std::shared_ptr<ImageView> image;
    };

    class SamplerDescriptor : public Descriptor
    {
      public:
        SamplerDescriptor(std::shared_ptr<Sampler> in_sampler) : sampler(std::move(in_sampler))
        {
        }

        VkWriteDescriptorSet     get() override;
        std::shared_ptr<Sampler> sampler;
    };

    std::unordered_map<std::string, std::shared_ptr<Descriptor>> write_descriptors;
    std::unordered_map<std::string, uint32_t>                    descriptor_bindings;

    std::vector<std::shared_ptr<Resource>> resources;

    std::weak_ptr<Device> device;
    bool                  b_static;
};
} // namespace Engine