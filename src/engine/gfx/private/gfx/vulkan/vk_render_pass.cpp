#include "gfx/vulkan/vk_render_pass.hpp"

#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/vk_check.hpp"

#include <array>

namespace Engine::Gfx
{

VkRendererPass::VkRendererPass(const std::string& name, const std::weak_ptr<Device>& in_device, RenderPass::Definition in_infos) : infos(std::move(in_infos)), device(in_device)
{
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference>   color_attachment_references;
    std::optional<VkAttachmentReference> depth_attachment_references;

    for (const auto& attachment : infos.attachments)
    {
        if (attachment.is_depth())
            depth_attachment_references = VkAttachmentReference{
                .attachment = static_cast<uint32_t>(attachments.size()),
                .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            };
        else
            color_attachment_references.emplace_back(VkAttachmentReference{
                .attachment = static_cast<uint32_t>(attachments.size()),
                .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            });

        attachments.emplace_back(VkAttachmentDescription{
            .format         = static_cast<VkFormat>(attachment.get_format()),
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = attachment.clear_value().is_none() ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = infos.present_pass ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        });
    }

    VkSubpassDescription subpass{
        .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount    = static_cast<uint32_t>(attachments.size()) - (depth_attachment_references ? 1 : 0),
        .pColorAttachments       = color_attachment_references.data(),
        .pDepthStencilAttachment = depth_attachment_references ? &*depth_attachment_references : nullptr,
    };

    constexpr std::array dependencies{VkSubpassDependency{
                                          .srcSubpass   = VK_SUBPASS_EXTERNAL, // Producer of the dependency
                                          .dstSubpass   = 0,                   // Consumer is our single subpass that will wait for the execution dependency
                                          .srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                          // Match our pWaitDstStageMask when we vkQueueSubmit
                                          .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                          // is a loadOp stage for color color_attachments
                                          .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT, // semaphore wait already does memory dependency for us
                                          .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                          // is a loadOp CLEAR access mask for color color_attachments
                                          .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
                                      },
                                      VkSubpassDependency{
                                          .srcSubpass   = 0,                   // Producer of the dependency is our single subpass
                                          .dstSubpass   = VK_SUBPASS_EXTERNAL, // Consumer are all commands outside the render pass
                                          .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                          // is a storeOp stage for color color_attachments
                                          .dstStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // Do not block any subsequent work
                                          .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                          // is a storeOp `STORE` access mask for color color_attachments
                                          .dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT,
                                          .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
                                      }};

    VkRenderPassCreateInfo renderPassInfo{
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments    = attachments.data(),
        .subpassCount    = 1,
        .pSubpasses      = &subpass,
        .dependencyCount = static_cast<uint32_t>(dependencies.size()),
        .pDependencies   = dependencies.data(),
    };

    VK_CHECK(vkCreateRenderPass(in_device.lock()->raw(), &renderPassInfo, nullptr, &ptr), "Failed to create render pass")
    in_device.lock()->debug_set_object_name(name, ptr);
}

VkRendererPass::~VkRendererPass()
{
    vkDestroyRenderPass(device.lock()->raw(), ptr, nullptr);
}
} // namespace Engine