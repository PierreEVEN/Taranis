#include <utility>

#include "gfx/vulkan/pipeline.hpp"
#include "gfx_types/pipeline.hpp"

#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/pipeline_layout.hpp"
#include "gfx/vulkan/shader_module.hpp"
#include "gfx/vulkan/vk_check.hpp"
#include "gfx/vulkan/vk_render_pass.hpp"
#include "gfx_types/format.hpp"

namespace Eng::Gfx
{
static VkPolygonMode vk_polygon_mode(EPolygonMode polygon_mode)
{
    switch (polygon_mode)
    {
    case EPolygonMode::Point:
        return VK_POLYGON_MODE_POINT;
    case EPolygonMode::Line:
        return VK_POLYGON_MODE_LINE;
    case EPolygonMode::Fill:
        return VK_POLYGON_MODE_FILL;
    default:
        LOG_FATAL("unhandled case")
    }
}

static VkPrimitiveTopology vk_topology(ETopology topology)
{
    switch (topology)
    {
    case ETopology::Points:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case ETopology::Lines:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case ETopology::Triangles:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    default:
        LOG_FATAL("unhandled case")
    }
}

static VkFrontFace vk_front_face(EFrontFace front_face)
{
    switch (front_face)
    {
    case EFrontFace::Clockwise:
        return VK_FRONT_FACE_CLOCKWISE;
    case EFrontFace::CounterClockwise:
        return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    default:
        LOG_FATAL("unhandled case")
    }
}

static VkCullModeFlags vk_cull_mode(ECulling culling, bool b_flip)
{
    switch (culling)
    {
    case ECulling::None:
        return VK_CULL_MODE_NONE;
    case ECulling::Front:
        return b_flip ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_FRONT_BIT;
    case ECulling::Back:
        return b_flip ? VK_CULL_MODE_FRONT_BIT : VK_CULL_MODE_BACK_BIT;
    case ECulling::Both:
        return VK_CULL_MODE_FRONT_AND_BACK;
    default:
        LOG_FATAL("unhandled case")
    }
}

Pipeline::Pipeline(std::string in_name, std::weak_ptr<Device> in_device, const std::weak_ptr<VkRendererPass>& render_pass, const std::vector<std::shared_ptr<ShaderModule>>& shader_stage, CreateInfos in_create_infos)
    : DeviceResource(std::move(in_name), std::move(in_device)), create_infos(std::move(in_create_infos))
{
    std::vector<VkVertexInputAttributeDescription> vertex_attribute_description;

    uint32_t vertex_input_size = 0;

    for (const auto& stage : shader_stage)
    {
        if (stage->infos().stage == EShaderStage::Vertex)
        {
            const auto input = create_infos.vertex_inputs;
            for (const auto& input_property : input)
            {
                vertex_attribute_description.emplace_back(VkVertexInputAttributeDescription{
                    .location = static_cast<uint32_t>(input_property.location),
                    .format = static_cast<VkFormat>(input_property.format),
                    .offset = input_property.offset,
                });

                vertex_input_size += get_format_channel_count(input_property.format) * get_format_bytes_per_pixel(input_property.format);
            }
        }
    }

    const VkVertexInputBindingDescription bindingDescription{
        .binding = 0,
        .stride = vertex_input_size,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    const VkPipelineVertexInputStateCreateInfo vertex_input_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescription.stride > 0 ? 1 : 0),
        .pVertexBindingDescriptions = bindingDescription.stride > 0 ? &bindingDescription : nullptr,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_description.size()),
        .pVertexAttributeDescriptions = vertex_attribute_description.data(),
    };

    const VkPipelineInputAssemblyStateCreateInfo input_assembly{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = vk_topology(create_infos.options.topology),
        .primitiveRestartEnable = VK_FALSE,
    };

    constexpr VkPipelineViewportStateCreateInfo viewport_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    const VkPipelineRasterizationStateCreateInfo rasterizer{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = vk_polygon_mode(create_infos.options.polygon),
        .cullMode = vk_cull_mode(create_infos.options.culling, render_pass.lock()->get_key().reverse_cull),
        .frontFace = vk_front_face(create_infos.options.front_face),
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = create_infos.options.line_width,
    };

    constexpr VkPipelineMultisampleStateCreateInfo multisampling{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    const VkPipelineDepthStencilStateCreateInfo depth_stencil{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = create_infos.options.depth_test,
        .depthWriteEnable = create_infos.options.depth_test,
        .depthCompareOp = create_infos.options.depth_test ? render_pass.lock()->get_key().b_reversed_z ? VK_COMPARE_OP_GREATER_OR_EQUAL : VK_COMPARE_OP_LESS : VK_COMPARE_OP_ALWAYS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };

    std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachment;
    for (const auto& attachment : render_pass.lock()->get_key().attachments)
    {
        if (is_depth_format(attachment.color_format))
        {
        }
        else
        {
            color_blend_attachment.emplace_back(VkPipelineColorBlendAttachmentState{
                .blendEnable = create_infos.options.alpha == EAlphaMode::Opaque ? VK_FALSE : VK_TRUE,
                .srcColorBlendFactor = create_infos.options.alpha == EAlphaMode::Opaque ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = create_infos.options.alpha == EAlphaMode::Opaque ? VK_BLEND_FACTOR_ZERO : VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = create_infos.options.alpha == EAlphaMode::Opaque ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            });
        }
    }

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    for (const auto& stage : shader_stage)
    {
        shaderStages.emplace_back(VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = static_cast<VkShaderStageFlagBits>(stage->infos().stage),
            .module = stage->raw(),
            .pName = "main",
        });
    }

    VkPipelineColorBlendStateCreateInfo color_blending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(color_blend_attachment.size()),
        .pAttachments = color_blend_attachment.data(),
    };

    std::vector dynamic_states_array = {VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT};
    if (create_infos.options.line_width != 1.0f)
        dynamic_states_array.emplace_back(VK_DYNAMIC_STATE_LINE_WIDTH);

    VkPipelineDynamicStateCreateInfo dynamic_states{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states_array.size()),
        .pDynamicStates = dynamic_states_array.data(),
    };

    layout = PipelineLayout::create(name(), device(), shader_stage);

    VkGraphicsPipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertex_input_state,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth_stencil,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_states,
        .layout = layout->raw(),
        .renderPass = render_pass.lock()->raw(),
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    VK_CHECK(vkCreateGraphicsPipelines(device().lock()->raw(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &ptr), "Failed to create material graphic pipeline")
    device().lock()->debug_set_object_name(name(), ptr);
}

Pipeline::~Pipeline()
{
    vkDestroyPipeline(device().lock()->raw(), ptr, nullptr);
}
} // namespace Eng::Gfx