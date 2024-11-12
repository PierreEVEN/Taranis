#include "gfx/vulkan/device.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "gfx/gfx.hpp"
#include "gfx/vulkan/descriptor_pool.hpp"
#include "gfx/vulkan/instance.hpp"
#include "gfx/vulkan/queue_family.hpp"
#include "gfx/vulkan/vk_render_pass.hpp"

#include <GLFW/glfw3.h>

namespace Eng::Gfx
{
const std::vector device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

void Device::next_frame()
{
    glfwPollEvents();
    current_image = (current_image + 1) % image_count;
}

void Device::wait() const
{
    vkDeviceWaitIdle(ptr);
}

void Device::flush_resources()
{
    std::lock_guard lock(resource_mutex);
    pending_kill_resources[current_image].clear();
}

Device::Device(const GfxConfig& config, const std::weak_ptr<Instance>& in_instance, const PhysicalDevice& physical_device, const Surface& surface)
    : queues(std::make_unique<Queues>(physical_device, surface)), physical_device(physical_device), instance(in_instance)
{
    float queuePriority = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> queues_info;
    for (const auto& queue : queues->all_families())
    {
        queues_info.emplace_back(VkDeviceQueueCreateInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, .queueFamilyIndex = queue->index(), .queueCount = 1, .pQueuePriorities = &queuePriority});
    }

    VkPhysicalDeviceFeatures deviceFeatures{
        .fillModeNonSolid  = true,
        .samplerAnisotropy = true,
    };
    VkDeviceCreateInfo createInfo{
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount    = static_cast<uint32_t>(queues_info.size()),
        .pQueueCreateInfos       = queues_info.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(device_extensions.size()),
        .ppEnabledExtensionNames = device_extensions.data(),
        .pEnabledFeatures        = &deviceFeatures,
    };

    b_enable_validation_layers = config.enable_validation_layers;
    if (config.enable_validation_layers)
    {
        createInfo.enabledLayerCount   = static_cast<uint32_t>(Instance::validation_layers().size());
        createInfo.ppEnabledLayerNames = Instance::validation_layers().data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    VK_CHECK(vkCreateDevice(physical_device.raw(), &createInfo, nullptr, &ptr), "Failed to create device")

    VmaAllocatorCreateInfo allocatorInfo = {
        .physicalDevice = physical_device.raw(),
        .device         = ptr,
        .instance       = instance.lock()->raw(),
    };
    VK_CHECK(vmaCreateAllocator(&allocatorInfo, &allocator), "failed to create vma allocator")
}

std::shared_ptr<Device> Device::create(const GfxConfig& config, const std::weak_ptr<Instance>& instance, const PhysicalDevice& physical_device, const Surface& surface)
{
    const auto device = std::shared_ptr<Device>(new Device(config, instance, physical_device, surface));
    for (const auto& queue : device->queues->all_families())
        queue->init_queue(device->weak_from_this());

    device->pending_kill_resources.resize(device->image_count, {});
    device->descriptor_pool = DescriptorPool::create(device);
    return device;
}

Device::~Device()
{
    vkDestroyDevice(ptr, nullptr);
}

const Queues& Device::get_queues() const
{
    return *queues;
}

const std::vector<const char*>& Device::get_device_extensions()
{
    return device_extensions;
}

std::weak_ptr<VkRendererPass> Device::find_or_create_render_pass(const RenderPassKey& key)
{
    const auto existing = render_passes.find(key);

    if (existing != render_passes.end())
        return existing->second;

    const auto new_render_pass = VkRendererPass::create(key.name + "_pass", shared_from_this(), key);
    render_passes.emplace(key, new_render_pass);
    return new_render_pass;
}

void Device::destroy_resources()
{
    wait();
    pending_kill_resources.clear();
    render_passes.clear();
    pending_kill_resources.clear();
    queues = nullptr;
    pending_kill_resources.clear();
    descriptor_pool = nullptr;

    VmaTotalStatistics stats;
    vmaCalculateStatistics(allocator, &stats);
    if (stats.total.statistics.allocationCount > 0)
        LOG_FATAL("{} allocation were not destroyed", stats.total.statistics.allocationCount)

    vmaDestroyAllocator(allocator);
    pending_kill_resources.clear();
}
} // namespace Eng::Gfx