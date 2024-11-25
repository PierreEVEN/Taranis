#include <utility>

#include "gfx/vulkan/shader_module.hpp"

#include "gfx/vulkan/device.hpp"

namespace Eng::Gfx
{
ShaderModule::ShaderModule(const std::weak_ptr<Device>& in_device, ShaderCompiler::StageData in_properties) : properties(std::move(in_properties)), device(in_device)
{
    assert(properties.compiled_module.size() % 4 == 0);
    const VkShaderModuleCreateInfo vertex_create_infos{
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = nullptr,
        .flags    = 0,
        .codeSize = properties.compiled_module.size() / 4,
        .pCode    = reinterpret_cast<uint32_t*>(properties.compiled_module.data()),
    };
    VK_CHECK(vkCreateShaderModule(device.lock()->raw(), &vertex_create_infos, nullptr, &ptr), "failed to create shader module")
}

ShaderModule::~ShaderModule()
{
    vkDestroyShaderModule(device.lock()->raw(), ptr, nullptr);
}
} // namespace Eng::Gfx
