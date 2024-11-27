#pragma once
#include "shader_compiler/shader_compiler.hpp"

#include <memory>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Eng::Gfx
{
class Device;

class ShaderModule
{
  public:
    static std::shared_ptr<ShaderModule> create(const std::weak_ptr<Device>& device, ShaderCompiler::StageData properties)
    {
        return std::shared_ptr<ShaderModule>(new ShaderModule(device, std::move(properties)));
    }

    ShaderModule(ShaderModule&)  = delete;
    ShaderModule(ShaderModule&&) = delete;
    ~ShaderModule();

    VkShaderModule raw() const
    {
        return ptr;
    }

    const ShaderCompiler::StageData& infos() const
    {
        return properties;
    }

  private:
    ShaderModule(const std::weak_ptr<Device>& device, ShaderCompiler::StageData properties);
    ShaderCompiler::StageData properties;
    VkShaderModule            ptr = VK_NULL_HANDLE;
    std::weak_ptr<Device>     device;
};
} // namespace Eng::Gfx