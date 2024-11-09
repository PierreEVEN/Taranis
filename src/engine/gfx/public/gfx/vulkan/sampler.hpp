#pragma once
#include <memory>
#include <string>
#include <vulkan/vulkan_core.h>

namespace Eng::Gfx
{
class Device;

class Sampler
{
  public:
    struct CreateInfos
    {
    };

    static std::shared_ptr<Sampler> create(const std::string& name, std::weak_ptr<Device> device, const CreateInfos& create_infos)
    {
        return std::shared_ptr<Sampler>(new Sampler(name, std::move(device), create_infos));
    }

    Sampler(Sampler&)  = delete;
    Sampler(Sampler&&) = delete;
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
    Sampler(const std::string& name, std::weak_ptr<Device> device, const CreateInfos& create_infos);
    std::weak_ptr<Device> device;
    VkSampler             ptr;
    VkDescriptorImageInfo descriptor_infos;
};
} // namespace Eng::Gfx
