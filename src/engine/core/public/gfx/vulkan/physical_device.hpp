#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#include "surface.hpp"
#include "swapchain.hpp"
#include "vk_check.hpp"

namespace Engine
{
class Config;
class Instance;

class PhysicalDevice
{
  public:
    SwapChainSupportDetails query_swapchain_support(const Surface& surface) const;
    PhysicalDevice(VkPhysicalDevice device);
    ~PhysicalDevice();

    static std::vector<PhysicalDevice> get_all_physical_devices(const Instance& instance);
    static Result<PhysicalDevice>      pick_best_physical_device(const std::weak_ptr<Instance>& instance, const Config& config, const std::shared_ptr<Surface>& surface);

    Result<int32_t> rate_device(const Config& config, const Surface& surface);

    std::string get_device_name() const;

    VkPhysicalDevice raw() const
    {
        return ptr;
    }

    bool does_support_debug_markers() const
    {
        return b_support_debug_markers;
    }

  private:
    bool check_extension_support();

    bool             b_support_debug_markers = false;
    VkPhysicalDevice ptr                     = VK_NULL_HANDLE;
};
} // namespace Engine
