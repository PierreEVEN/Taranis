#include "gfx/vulkan/physical_device.hpp"

#include <map>
#include <set>

#include "gfx/gfx.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/instance.hpp"
#include "gfx/vulkan/swapchain.hpp"
#include "gfx/window.hpp"

namespace Engine::Gfx
{
class Config;

SwapChainSupportDetails PhysicalDevice::query_swapchain_support(const Surface& surface) const
{
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ptr, surface.raw(), &details.capabilities);
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ptr, surface.raw(), &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(ptr, surface.raw(), &formatCount, details.formats.data());
    }
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(ptr, surface.raw(), &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(ptr, surface.raw(), &presentModeCount, details.presentModes.data());
    }
    return details;
}

PhysicalDevice::PhysicalDevice(VkPhysicalDevice device) : ptr(device)
{
}

PhysicalDevice::~PhysicalDevice() = default;

std::vector<PhysicalDevice> PhysicalDevice::get_all_physical_devices(const Instance& instance)
{
    std::vector<PhysicalDevice> found_devices;
    uint32_t                    deviceCount = 0;
    vkEnumeratePhysicalDevices(instance.raw(), &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance.raw(), &deviceCount, devices.data());
    for (const auto& device : devices)
        found_devices.emplace_back(device);
    return found_devices;
}

Result<PhysicalDevice> PhysicalDevice::pick_best_physical_device(const std::weak_ptr<Instance>& instance, const GfxConfig& config, const std::shared_ptr<Surface>& surface)
{
    std::vector<PhysicalDevice>            devices = get_all_physical_devices(*instance.lock());
    std::multimap<int32_t, PhysicalDevice> candidates;
    std::vector<std::string>               errors;

    for (auto& device : devices)
    {
        auto score = device.rate_device(config, *surface);
        if (score)
            candidates.insert(std::make_pair(score.get(), device));
        else
            errors.emplace_back(device.get_device_name() + " : " + score.error());
    }

    if (candidates.empty())
    {
        std::string message = "There is no suitable physical device :";
        for (const auto& error : errors)
            message += error + "\n";
        return Result<PhysicalDevice>::Error(message);
    }

    return Result<PhysicalDevice>::Ok(candidates.begin()->second);
}

Result<int32_t> PhysicalDevice::rate_device(const GfxConfig& config, const Surface& surface)
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(ptr, &deviceProperties);
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(ptr, &deviceFeatures);

    if (config.allow_integrated_gpus && deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        return Result<int32_t>::Error("This device is an integrated GPU but integrated gpu are not currently allowed");

    if (!check_extension_support())
        return Result<int32_t>::Error("This device doesn't support required extensions");

    SwapChainSupportDetails swapChainSupport = query_swapchain_support(surface);
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
        return Result<int32_t>::Error("This device doesn't support required swapchain configuration");

    int32_t score = 0;

    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 1000;
    }

    score += deviceProperties.limits.maxImageDimension2D;

    return Result<int32_t>::Ok(score);
}

std::string PhysicalDevice::get_device_name() const
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(ptr, &deviceProperties);
    return deviceProperties.deviceName;
}

bool PhysicalDevice::check_extension_support()
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(ptr, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(ptr, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(Device::get_device_extensions().begin(), Device::get_device_extensions().end());

    for (const auto& extension : availableExtensions)
    {
        if (extension.extensionName == std::string(VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
            b_support_debug_markers = true;
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}
} // namespace Engine::Gfx
