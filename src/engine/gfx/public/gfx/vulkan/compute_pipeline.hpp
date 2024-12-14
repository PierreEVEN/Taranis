#pragma once

#include "device_resource.hpp"
#include "pipeline.hpp"

#include <memory>
#include <string>

namespace Eng::Gfx
{
class PipelineLayout;
}

namespace Eng::Gfx
{
class Device;
class ShaderModule;
class VkRendererPass;

class ComputePipeline : public DeviceResource
{
public:
    struct CreateInfos
    {
    };

    static std::shared_ptr<ComputePipeline> create(const std::string& name, std::weak_ptr<Device> in_device, const std::shared_ptr<ShaderModule>& shader_stage, CreateInfos in_create_infos = {})
    {
        return std::shared_ptr<ComputePipeline>(new ComputePipeline(name, std::move(in_device), shader_stage, std::move(in_create_infos)));
    }

    VkPipeline raw() const
    {
        return ptr;
    }

    const std::shared_ptr<PipelineLayout>& get_layout() const
    {
        return layout;
    }

private:
    ComputePipeline(const std::string& name, std::weak_ptr<Device> in_device, const std::shared_ptr<ShaderModule>& shader_stage, CreateInfos in_create_infos);

    std::shared_ptr<PipelineLayout> layout;
    VkPipeline                      ptr;
};
} // namespace Eng::Gfx