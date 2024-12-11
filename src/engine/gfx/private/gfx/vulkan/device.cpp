#include "gfx/vulkan/device.hpp"

#define VMA_IMPLEMENTATION
#include "profiler.hpp"

#include <vk_mem_alloc.h>

#include "gfx/gfx.hpp"
#include "gfx/vulkan/descriptor_pool.hpp"
#include "gfx/vulkan/instance.hpp"
#include "gfx/vulkan/queue_family.hpp"
#include "gfx/vulkan/vk_render_pass.hpp"
#include "gfx/vulkan/vk_wrap.hpp"

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
    PROFILER_SCOPE(DeviceWaitIdle);
    std::unique_lock lk(queues->global_lock());
    vkDeviceWaitIdle(ptr);
}

void Device::flush_resources()
{
    PROFILER_SCOPE(FlushResources);
    std::lock_guard lock(resource_mutex);
    pending_kill_resources[current_image].clear();
}

Device::Device(const GfxConfig& in_config, const std::weak_ptr<Instance>& in_instance, const PhysicalDevice& physical_device, const Surface& surface)
    : queues(std::make_unique<Queues>(physical_device, surface)), physical_device(physical_device), instance(in_instance), config(in_config)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physical_device.raw(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        std::string type_infos;
        auto        flags = memProperties.memoryTypes[i].propertyFlags;
        if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            type_infos += "| device-local";
        if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            type_infos += "| host-visible";
        if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
            type_infos += "| host-coherent";
        if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
            type_infos += "| host-cached";
        if (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
            type_infos += "| lazily-allocated";
        if (flags & VK_MEMORY_PROPERTY_PROTECTED_BIT)
            type_infos += "| protected";
        if (flags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
            type_infos += "| device-coherent";
        if (flags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)
            type_infos += "| device-uncached";
        if (flags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV)
            type_infos += "| rdma-capable";

        LOG_INFO("Memory type {} : {}", i, type_infos);
    }

    float queuePriority = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> queues_info;
    for (const auto& queue : queues->all_families())
    {
        queues_info.emplace_back(VkDeviceQueueCreateInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, .queueFamilyIndex = queue->index(), .queueCount = 1, .pQueuePriorities = &queuePriority});
    }

    VkPhysicalDeviceFeatures deviceFeatures{
        .fillModeNonSolid = true,
        .samplerAnisotropy = true,
    };

    VkPhysicalDeviceVulkan12Features device_features_12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorBindingPartiallyBound = true,
        .descriptorBindingVariableDescriptorCount = true,
        .runtimeDescriptorArray = true,
    };

    VkDeviceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &device_features_12,
        .queueCreateInfoCount = static_cast<uint32_t>(queues_info.size()),
        .pQueueCreateInfos = queues_info.data(),
        .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
        .ppEnabledExtensionNames = device_extensions.data(),
        .pEnabledFeatures = &deviceFeatures,
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
        .device = ptr,
        .instance = instance.lock()->raw(),
    };
    VmaAllocator vma_alloc;
    VK_CHECK(vmaCreateAllocator(&allocatorInfo, &vma_alloc), "failed to create vma allocator")
    allocator = std::make_unique<VmaAllocatorWrap>(vma_alloc);
}

std::shared_ptr<Device> Device::create(const GfxConfig& config, const std::weak_ptr<Instance>& instance, const PhysicalDevice& physical_device, const Surface& surface)
{
    PROFILER_SCOPE(CreateDevice);
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

const VmaAllocatorWrap& Device::get_allocator() const
{
    return *allocator;
}

const std::vector<const char*>& Device::get_device_extensions()
{
    return device_extensions;
}

std::weak_ptr<VkRendererPass> Device::declare_render_pass(const RenderPassKey& key, const RenderPassGenericId& name)
{
    registered_render_passes.emplace(key.render_pass_ref.generic_id(), ankerl::unordered_dense::set<RenderPassRef>{}).first->second.insert(key.render_pass_ref);
    const auto existing = render_passes.find(key);

    if (existing != render_passes.end())
    {
        render_passes_named.emplace(name, existing->second);
        return existing->second;
    }

    const auto new_render_pass = VkRendererPass::create(key.render_pass_ref.generic_id(), shared_from_this(), key);
    render_passes.emplace(key, new_render_pass);
    render_passes_named.emplace(name, new_render_pass);
    return new_render_pass;
}

std::weak_ptr<VkRendererPass> Device::get_render_pass(const RenderPassGenericId& name) const
{
    if (auto found = render_passes_named.find(name); found != render_passes_named.end())
        return found->second;
    return {};
}

void Device::destroy_resources()
{
    wait();
    pending_kill_resources.clear();
    render_passes.clear();
    render_passes_named.clear();
    pending_kill_resources.clear();
    queues = nullptr;
    pending_kill_resources.clear();
    descriptor_pool = nullptr;

    VmaTotalStatistics stats;
    vmaCalculateStatistics(allocator->allocator, &stats);
    if (stats.total.statistics.allocationCount > 0)
        LOG_ERROR("{} allocation were not destroyed", stats.total.statistics.allocationCount);

    vmaDestroyAllocator(allocator->allocator);
    pending_kill_resources.clear();
}
} // namespace Eng::Gfx