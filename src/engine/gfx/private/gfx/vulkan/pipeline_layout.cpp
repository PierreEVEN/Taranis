#include "gfx/vulkan/pipeline_layout.hpp"

#include "logger.hpp"
#include "gfx/vulkan/descriptor_pool.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/shader_module.hpp"

namespace Eng::Gfx
{

static VkDescriptorType vk_descriptor_type(EBindingType type)
{
    switch (type)
    {
    case EBindingType::SAMPLER:
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    case EBindingType::COMBINED_IMAGE_SAMPLER:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case EBindingType::SAMPLED_IMAGE:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case EBindingType::STORAGE_IMAGE:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case EBindingType::UNIFORM_TEXEL_BUFFER:
        return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    case EBindingType::STORAGE_TEXEL_BUFFER:
        return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    case EBindingType::UNIFORM_BUFFER:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case EBindingType::STORAGE_BUFFER:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case EBindingType::UNIFORM_BUFFER_DYNAMIC:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    case EBindingType::STORAGE_BUFFER_DYNAMIC:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    case EBindingType::INPUT_ATTACHMENT:
        return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    default: ;
        LOG_FATAL("unhandled case")
    }
}

PipelineLayout::~PipelineLayout()
{
    vkDestroyDescriptorSetLayout(device().lock()->raw(), descriptor_layout, nullptr);
    vkDestroyPipelineLayout(device().lock()->raw(), ptr, nullptr);
}

PipelineLayout::PipelineLayout(std::string in_name, std::weak_ptr<Device> in_device, const std::vector<std::shared_ptr<ShaderModule>>& shader_stage) : DeviceResource(std::move(in_name), std::move(in_device))
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::vector<VkDescriptorBindingFlags>     flags{};
    bool                                      bindless = false;
    for (const auto& stage : shader_stage)
    {
        for (const auto& binding : stage->infos().bindings)
        {
            if (bindless)
                LOG_FATAL("Variable sized descriptor {} should be the last one", binding.name);
            descriptor_bindings.push_back(binding);
            bindings.emplace_back(VkDescriptorSetLayoutBinding{
                .binding = binding.binding,
                .descriptorType = vk_descriptor_type(binding.type),
                .descriptorCount = binding.array_elements > 0 ? binding.array_elements : 1,
                .stageFlags = static_cast<VkShaderStageFlags>(stage->infos().stage),
                .pImmutableSamplers = nullptr,
            });
            if (binding.array_elements > 0)
                flags.emplace_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
            else
                flags.emplace_back(0);

            if (binding.array_elements == UINT32_MAX)
            {
                bindless = true;
                LOG_ERROR("Bindless descriptors are not supported yet");
                flags.back() |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
            }
        }
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindingFlags = flags.data(),
    };

    VkDescriptorSetLayoutCreateInfo layout_infos{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &bindingFlags,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    VK_CHECK(vkCreateDescriptorSetLayout(device().lock()->raw(), &layout_infos, nullptr, &descriptor_layout), "Failed to create descriptor set layout")
    device().lock()->debug_set_object_name(name() + "_set_layout", descriptor_layout);

    std::vector<VkPushConstantRange> push_constants = {};
    for (const auto& stage : shader_stage)
        if (stage->infos().push_constant_size > 0)
            push_constants.emplace_back(VkPushConstantRange{
                .stageFlags = static_cast<VkShaderStageFlags>(stage->infos().stage),
                .offset = 0,
                .size = stage->infos().push_constant_size,
            });

    const VkPipelineLayoutCreateInfo pipeline_layout_infos{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_layout,
        .pushConstantRangeCount = static_cast<uint32_t>(push_constants.size()),
        .pPushConstantRanges = push_constants.data(),
    };
    VK_CHECK(vkCreatePipelineLayout(device().lock()->raw(), &pipeline_layout_infos, nullptr, &ptr), "Failed to create pipeline layout")
    device().lock()->debug_set_object_name(name() + "_layout", ptr);
}
}