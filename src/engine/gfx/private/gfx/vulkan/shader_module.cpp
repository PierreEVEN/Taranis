#include <utility>

#include "gfx/vulkan/shader_module.hpp"

#include "gfx/vulkan/device.hpp"

namespace Eng::Gfx
{
ShaderModule::ShaderModule(const std::weak_ptr<Device>& in_device, ShaderProperties in_properties) : properties(std::move(in_properties)), device(in_device)
{
    const VkShaderModuleCreateInfo vertex_create_infos{
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = nullptr,
        .flags    = 0,
        .codeSize = properties.spirv.size(),
        .pCode    = reinterpret_cast<uint32_t*>(properties.spirv.data()),
    };
    VK_CHECK(vkCreateShaderModule(device.lock()->raw(), &vertex_create_infos, nullptr, &ptr), "failed to create vertex shader module")
}

ShaderModule::~ShaderModule()
{
    vkDestroyShaderModule(device.lock()->raw(), ptr, nullptr);
}
} // namespace Eng::Gfx
