#pragma once
#include <memory>
#include <string>
#include <vector>
#include <glm/vec4.hpp>
#include <vulkan/vulkan_core.h>

namespace Eng::Gfx
{
class GfxConfig;

class Instance
{
  public:
    static std::shared_ptr<Instance> create(GfxConfig& config)
    {
        return std::shared_ptr<Instance>(new Instance(config));
    }

    Instance(Instance&&) = delete;
    Instance(Instance&)  = delete;
    ~Instance();

    [[nodiscard]] VkInstance raw() const
    {
        return ptr;
    }

    static const std::vector<const char*>& validation_layers();

    void begin_debug_marker(const VkCommandBuffer& cmd, const std::string& name, const std::array<float, 4>& color) const;
    void end_debug_marker(const VkCommandBuffer& cmd) const;

  private:
    Instance(GfxConfig& config);
    VkInstance               ptr             = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

    static bool                     are_validation_layer_supported();
    static std::vector<const char*> get_required_extensions(const GfxConfig& config);
};
} // namespace Eng::Gfx
