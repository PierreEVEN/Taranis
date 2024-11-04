#pragma once
#include <memory>
#include <vulkan/vulkan_core.h>

namespace Engine
{
class Device;

class Sampler
{
  public:
    struct CreateInfos
    {
    };

    Sampler(std::weak_ptr<Device> device, const CreateInfos& create_infos);
    ~Sampler();
    VkSampler raw() const
    {
        return ptr;
    }

    const VkDescriptorImageInfo& get_descriptor_infos() const
    {
        return descriptor_infos;
    }

  private:
    std::weak_ptr<Device> device;
    VkSampler             ptr;
    VkDescriptorImageInfo descriptor_infos;
};
} // namespace Engine
