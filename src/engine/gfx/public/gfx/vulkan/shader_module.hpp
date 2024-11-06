#pragma once
#include <memory>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "gfx/shaders/shader_compiler.hpp"

namespace Engine
{
class Device;

class ShaderModule
{
public:
    static std::shared_ptr<ShaderModule> create(const std::weak_ptr<Device>& device, ShaderProperties properties)
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

    const ShaderProperties& infos() const
    {
        return properties;
    }

private:
    ShaderModule(const std::weak_ptr<Device>& device, ShaderProperties properties);
    ShaderProperties      properties;
    VkShaderModule        ptr = VK_NULL_HANDLE;
    std::weak_ptr<Device> device;
};
} // namespace Engine