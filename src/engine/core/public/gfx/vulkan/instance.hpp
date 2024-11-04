#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Engine
{
class Config;

class Instance
{
  public:
    Instance(Config& config);
    ~Instance();

    [[nodiscard]] VkInstance raw() const
    {
        return ptr;
    }

    static const std::vector<const char*>& validation_layers();

  private:
    VkInstance               ptr             = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

    static bool                     are_validation_layer_supported();
    static std::vector<const char*> get_required_extensions(const Config& config);
};
} // namespace Engine
