#include "gfx/vulkan/sampler.hpp"

#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/vk_check.hpp"

namespace Engine::Gfx
{
Sampler::Sampler(const std::string& name, std::weak_ptr<Device> in_device, const CreateInfos&) : device(std::move(in_device))
{
    constexpr VkSamplerCreateInfo sampler_create_infos{
        .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0,
        .magFilter               = VK_FILTER_LINEAR,
        .minFilter               = VK_FILTER_LINEAR,
        .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias              = 0.f,
        .anisotropyEnable        = VK_TRUE,
        .maxAnisotropy           = 16,
        .compareEnable           = VK_FALSE,
        .compareOp               = VK_COMPARE_OP_ALWAYS,
        .minLod                  = 0.0f,
        .maxLod                  = 0.0f,
        .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };
    VK_CHECK(vkCreateSampler(device.lock()->raw(), &sampler_create_infos, nullptr, &ptr), "failed to create sampler")
    device.lock()->debug_set_object_name(name, ptr);

    descriptor_infos = {
        .sampler     = ptr,
        .imageView   = VK_NULL_HANDLE,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
}

Sampler::~Sampler()
{
    vkDestroySampler(device.lock()->raw(), ptr, nullptr);
}
} // namespace Engine::Gfx
