#include "gfx/vulkan/compute_pipeline.hpp"

#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/pipeline_layout.hpp"
#include "gfx/vulkan/shader_module.hpp"

namespace Eng::Gfx
{
ComputePipeline::ComputePipeline(std::string in_name, std::weak_ptr<Device> in_device, const std::shared_ptr<ShaderModule>& shader_stage, CreateInfos) : DeviceResource(std::move(in_name), std::move(in_device))
{
    layout = PipelineLayout::create(name(), device(), {shader_stage});

    VkPipelineShaderStageCreateInfo computeShaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shader_stage->raw(),
        .pName = "main",
    };

    VkComputePipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = computeShaderStageInfo,
        .layout = layout->raw(),
    };
    vkCreateComputePipelines(device().lock()->raw(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &ptr);
    device().lock()->debug_set_object_name(name(), ptr);
}

ComputePipeline::~ComputePipeline()
{
    vkDestroyPipeline(device().lock()->raw(), ptr, VK_NULL_HANDLE);
    ptr = VK_NULL_HANDLE;
}
}