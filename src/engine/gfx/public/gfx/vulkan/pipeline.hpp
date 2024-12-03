#pragma once
#include "device.hpp"
#include "gfx_types/pipeline.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Eng::Gfx
{
enum class ColorFormat;
}

namespace ShaderCompiler
{
struct BindingDescription;
}

struct StageInputOutputDescription
{
    StageInputOutputDescription(uint32_t in_location, uint32_t in_offset, Eng::Gfx::ColorFormat in_format) : location(in_location), offset(in_offset), format(in_format)
    {
    }

    uint32_t              location;
    uint32_t              offset;
    Eng::Gfx::ColorFormat format;
};

namespace Eng::Gfx
{
class ShaderModule;
class Device;
class VkRendererPass;
enum class ColorFormat;

class Pipeline : public DeviceResource
{
  public:
    struct CreateInfos
    {
        PipelineOptions                          options;
        std::vector<StageInputOutputDescription> vertex_inputs;
    };

    static std::shared_ptr<Pipeline> create(const std::string& name, std::weak_ptr<Device> device, const std::weak_ptr<VkRendererPass>& render_pass, const std::vector<std::shared_ptr<ShaderModule>>& shader_stage,
                                            CreateInfos create_infos)
    {
        return std::shared_ptr<Pipeline>(new Pipeline(name, std::move(device), render_pass, shader_stage, std::move(create_infos)));
    }

    Pipeline(Pipeline&)  = delete;
    Pipeline(Pipeline&&) = delete;
    ~Pipeline();

    const VkPipelineLayout& get_layout() const
    {
        return layout;
    }

    const VkDescriptorSetLayout& get_descriptor_layout() const
    {
        return descriptor_set_layout;
    }

    const VkPipeline& raw() const
    {
        return ptr;
    }

    const CreateInfos& infos() const
    {
        return create_infos;
    }

    const std::vector<ShaderCompiler::BindingDescription>& get_bindings() const
    {
        return descriptor_bindings;
    }

  private:
    Pipeline(const std::string& name, std::weak_ptr<Device> device, const std::weak_ptr<VkRendererPass>& render_pass, const std::vector<std::shared_ptr<ShaderModule>>& shader_stage, CreateInfos create_infos);
    std::vector<ShaderCompiler::BindingDescription> descriptor_bindings;
    CreateInfos                                     create_infos;
    VkPipelineLayout                                layout                = VK_NULL_HANDLE;
    VkDescriptorSetLayout                           descriptor_set_layout = VK_NULL_HANDLE;
    VkPipeline                                      ptr                   = VK_NULL_HANDLE;
};
} // namespace Eng::Gfx