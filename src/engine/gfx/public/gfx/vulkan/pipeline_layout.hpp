#pragma once
#include "device_resource.hpp"
#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace ShaderCompiler
{
struct BindingDescription;
}

namespace Eng::Gfx
{
class ShaderModule;

class PipelineLayout : public DeviceResource
{
public:
    static std::shared_ptr<PipelineLayout> create(const std::string& name, std::weak_ptr<Device> in_device, const std::vector<std::shared_ptr<ShaderModule>>& shader_stage)
    {
        return std::shared_ptr<PipelineLayout>(new PipelineLayout(name, std::move(in_device), shader_stage));
    }

    ~PipelineLayout();

    const std::vector<ShaderCompiler::BindingDescription>& get_bindings() const
    {
        return descriptor_bindings;
    }

    const VkPipelineLayout& raw() const
    {
        return ptr;
    }

    const VkDescriptorSetLayout& get_descriptor_layout() const
    {
        return descriptor_layout;
    }

private:
    PipelineLayout(const std::string& name, std::weak_ptr<Device> in_device, const std::vector<std::shared_ptr<ShaderModule>>& shader_stage);

    VkDescriptorSetLayout                           descriptor_layout = VK_NULL_HANDLE;
    VkPipelineLayout                                ptr               = VK_NULL_HANDLE;
    std::vector<ShaderCompiler::BindingDescription> descriptor_bindings;
};
}