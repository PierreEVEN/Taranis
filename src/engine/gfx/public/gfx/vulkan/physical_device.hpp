#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#include "surface.hpp"
#include "vk_check.hpp"

namespace Eng::Gfx
{
class GfxConfig;

class Instance;

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

class PhysicalDevice
{
  public:
    SwapChainSupportDetails query_swapchain_support(const Surface& surface) const;
    PhysicalDevice(VkPhysicalDevice device);
    ~PhysicalDevice();

    static std::vector<PhysicalDevice> get_all_physical_devices(const Instance& instance);
    static Result<PhysicalDevice>      pick_best_physical_device(const std::weak_ptr<Instance>& instance, const GfxConfig& config, const std::shared_ptr<Surface>& surface);

    Result<int32_t> rate_device(const GfxConfig& config, const Surface& surface);

    std::string get_device_name() const;

    VkPhysicalDevice raw() const
    {
        return ptr;
    }

  private:
    bool check_extension_support();

    VkPhysicalDevice ptr = VK_NULL_HANDLE;
};
} // namespace Eng::Gfx
