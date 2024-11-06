#pragma once
#include "instance.hpp"
#include <unordered_map>

#include "gfx/renderer/definition/render_pass.hpp"
#include "physical_device.hpp"

#include <vk_mem_alloc.h>

namespace Engine::Gfx
{
class VkRendererPass;
class DescriptorPool;
class DeviceResource;
class Queues;

class Device : public std::enable_shared_from_this<Device>
{
    friend class Engine;

  public:
    static std::shared_ptr<Device> create(const GfxConfig& config, const std::weak_ptr<Instance>& instance, const PhysicalDevice& physical_device, const Surface& surface);
    Device(Device&)  = delete;
    Device(Device&&) = delete;
    ~Device();

    VkDevice raw() const
    {
        return ptr;
    }

    const Queues& get_queues() const;

    const VmaAllocator& get_allocator() const
    {
        return allocator;
    }

    const PhysicalDevice& get_physical_device() const
    {
        return physical_device;
    }

    static const std::vector<const char*>& get_device_extensions();
    std::shared_ptr<VkRendererPass>        find_or_create_render_pass(const RenderPass::Definition& infos);

    void destroy_resources();

    uint32_t get_image_count() const
    {
        return image_count;
    }

    uint32_t get_current_image() const
    {
        return current_image;
    }

    void next_frame();

    void wait() const;

    void flush_resources();

    void drop_resource(const std::shared_ptr<DeviceResource>& resource)
    {
        std::lock_guard lock(resource_mutex);
        if (current_image > pending_kill_resources.size())
            pending_kill_resources.resize(current_image);
        pending_kill_resources[current_image].emplace_back(resource);
    }

    DescriptorPool& get_descriptor_pool() const
    {
        return *descriptor_pool;
    }

    template <typename Object_T> void debug_set_object_name([[maybe_unused]] const std::string& object_name, [[maybe_unused]] const Object_T& object)
    {
        if (b_enable_validation_layers)
        {
            VkObjectType object_type = VK_OBJECT_TYPE_UNKNOWN;
            if (typeid(VkInstance) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_INSTANCE;
            else if (typeid(VkPhysicalDevice) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_PHYSICAL_DEVICE;
            else if (typeid(VkDevice) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_DEVICE;
            else if (typeid(VkQueue) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_QUEUE;
            else if (typeid(VkSemaphore) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_SEMAPHORE;
            else if (typeid(VkCommandBuffer) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_COMMAND_BUFFER;
            else if (typeid(VkFence) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_FENCE;
            else if (typeid(VkDeviceMemory) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_DEVICE_MEMORY;
            else if (typeid(VkBuffer) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_BUFFER;
            else if (typeid(VkImage) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_IMAGE;
            else if (typeid(VkEvent) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_EVENT;
            else if (typeid(VkQueryPool) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_QUERY_POOL;
            else if (typeid(VkBufferView) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_BUFFER_VIEW;
            else if (typeid(VkImageView) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_IMAGE_VIEW;
            else if (typeid(VkShaderModule) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_SHADER_MODULE;
            else if (typeid(VkPipelineCache) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_PIPELINE_CACHE;
            else if (typeid(VkPipelineLayout) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_PIPELINE_LAYOUT;
            else if (typeid(VkRenderPass) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_RENDER_PASS;
            else if (typeid(VkPipeline) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_PIPELINE;
            else if (typeid(VkDescriptorSetLayout) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
            else if (typeid(VkSampler) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_SAMPLER;
            else if (typeid(VkDescriptorPool) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_DESCRIPTOR_POOL;
            else if (typeid(VkDescriptorPool) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_DESCRIPTOR_SET;
            else if (typeid(VkFramebuffer) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_FRAMEBUFFER;
            else if (typeid(VkCommandPool) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_COMMAND_POOL;
            else if (typeid(VkSurfaceKHR) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_SURFACE_KHR;
            else if (typeid(VkSwapchainKHR) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_SWAPCHAIN_KHR;
            else if (typeid(VkDescriptorSet) == typeid(Object_T))
                object_type = VK_OBJECT_TYPE_DESCRIPTOR_SET;
            else
                LOG_FATAL("unhandled debug object type : {}", typeid(Object_T).name())

            const auto                    pfn_vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(instance.lock()->raw(), "vkSetDebugUtilsObjectNameEXT"));
            VkDebugUtilsObjectNameInfoEXT object_name_info                 = {
                                .sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                                .pNext        = nullptr,
                                .objectType   = object_type,
                                .objectHandle = reinterpret_cast<uint64_t>(object),
                                .pObjectName  = object_name.c_str(),
            };
            pfn_vkSetDebugUtilsObjectNameEXT(ptr, &object_name_info);
        }
    }

    void debug_add_marker([[maybe_unused]] const std::string& marker_name, [[maybe_unused]] VkCommandBuffer command_buffer, [[maybe_unused]] std::array<float, 4> color)
    {
        if (b_enable_validation_layers && physical_device.does_support_debug_markers())
        {
            // add marker
            const auto                 pfn_debug_marker_begin = reinterpret_cast<PFN_vkCmdDebugMarkerBeginEXT>(vkGetDeviceProcAddr(ptr, "vkCmdDebugMarkerBeginEXT"));
            VkDebugMarkerMarkerInfoEXT begin_marker           = {
                          .sType       = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT,
                          .pNext       = nullptr,
                          .pMarkerName = marker_name.c_str(),
            };
            memcpy(begin_marker.color, color.data(), sizeof(float) * 4);
            pfn_debug_marker_begin(command_buffer, &begin_marker);
        }
    }

    void debug_end_marker([[maybe_unused]] VkCommandBuffer command_buffer)
    {
        if (b_enable_validation_layers && physical_device.does_support_debug_markers())
        {
            const auto pfn_debug_marker_end = reinterpret_cast<PFN_vkCmdDebugMarkerEndEXT>(vkGetDeviceProcAddr(ptr, "vkCmdDebugMarkerEndEXT"));
            pfn_debug_marker_end(command_buffer);
        }
    }

  private:
    bool b_enable_validation_layers = false;
    Device(const GfxConfig& config, const std::weak_ptr<Instance>& instance, const PhysicalDevice& physical_device, const Surface& surface);
    std::unordered_map<RenderPass::Definition, std::shared_ptr<VkRendererPass>> render_passes;
    std::unique_ptr<Queues>                                                     queues;
    PhysicalDevice                                                              physical_device;
    VkDevice                                                                    ptr = VK_NULL_HANDLE;
    VmaAllocator                                                                allocator;
    uint32_t                                                                    image_count   = 2;
    uint32_t                                                                    current_image = 0;
    std::mutex                                                                  resource_mutex;
    std::vector<std::vector<std::shared_ptr<DeviceResource>>>                   pending_kill_resources;
    std::shared_ptr<DescriptorPool>                                             descriptor_pool;
    std::weak_ptr<Instance>                                                     instance;
};

class DeviceResource : public std::enable_shared_from_this<DeviceResource>
{
  public:
    DeviceResource(std::weak_ptr<Device> in_device) : device_ref(std::move(in_device))
    {
    }

    const std::weak_ptr<Device>& device() const
    {
        return device_ref;
    }

  private:
    std::weak_ptr<Device> device_ref;
};
} // namespace Engine