#include "gfx/vulkan/swapchain.hpp"

#include "gfx/renderer/definition/renderer.hpp"
#include "gfx/ui/ImGuiWrapper.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/fence.hpp"
#include "gfx/vulkan/framebuffer.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/queue_family.hpp"
#include "gfx/vulkan/semaphore.hpp"
#include "gfx/vulkan/surface.hpp"
#include "gfx/window.hpp"
#include "profiler.hpp"

namespace Eng::Gfx
{
Swapchain::Swapchain(const std::weak_ptr<Device>& in_device, const std::weak_ptr<Surface>& in_surface, const Renderer& renderer)
    : RenderPassInstance(in_device, renderer.compile(get_swapchain_format(in_device, in_surface), in_device), *renderer.root_node(), true), surface(in_surface)
{
    create_or_recreate();
}

Swapchain::~Swapchain()
{
    destroy();
    image_view = nullptr;
    swapChainImages.clear();
}

std::vector<const Semaphore*> Swapchain::get_semaphores_to_wait(SwapchainImageId swapchain_image) const
{
    auto semaphores = RenderPassInstance::get_semaphores_to_wait(device.lock()->get_current_image());
    semaphores.push_back(image_available_semaphores[swapchain_image].get());
    return semaphores;
}

ColorFormat Swapchain::get_swapchain_format(const std::weak_ptr<Device>& in_device, const std::weak_ptr<Surface>& surface)
{
    SwapChainSupportDetails swapchain_support = in_device.lock()->get_physical_device().query_swapchain_support(*surface.lock());
    return static_cast<ColorFormat>(choose_surface_format(swapchain_support.formats).format);
}

VkSurfaceFormatKHR Swapchain::choose_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
    for (const auto& availableFormat : available_formats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
            return availableFormat;

        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;
    }

    return available_formats[0];
}

VkPresentModeKHR Swapchain::choose_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
    for (const auto& availablePresentMode : available_present_modes)
    {
        if (device.lock()->get_gfx_config().v_sync)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                return availablePresentMode;
        }
        else
        {
            if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

glm::uvec2 Swapchain::choose_extent(const VkSurfaceCapabilitiesKHR& capabilities, glm::uvec2 base_extent)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return glm::uvec2{capabilities.currentExtent.width, capabilities.currentExtent.height};
    base_extent.x = std::clamp(base_extent.x, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    base_extent.y = std::clamp(base_extent.y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return base_extent;
}

void Swapchain::create_or_recreate()
{
    const auto              device_ptr        = device.lock();
    SwapChainSupportDetails swapchain_support = device_ptr->get_physical_device().query_swapchain_support(*surface.lock());

    const auto window_extent = surface.lock()->get_window().lock()->internal_extent();
    auto       new_extent    = choose_extent(swapchain_support.capabilities, window_extent);
    if (new_extent.x <= 0 || new_extent.y <= 0)
        return;
    destroy();

    PROFILER_SCOPE(RecreateSwapchain);
    VkPresentModeKHR   presentMode   = choose_present_mode(swapchain_support.presentModes);
    VkSurfaceFormatKHR surfaceFormat = choose_surface_format(swapchain_support.formats);
    swapchain_format                 = static_cast<ColorFormat>(surfaceFormat.format);
    extent                           = new_extent;
    uint32_t imageCount              = get_framebuffer_count();

    if (swapchain_support.capabilities.maxImageCount > 0 && imageCount > swapchain_support.capabilities.maxImageCount)
    {
        imageCount = swapchain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = surface.lock()->raw(),
        .minImageCount    = imageCount,
        .imageFormat      = surfaceFormat.format,
        .imageColorSpace  = surfaceFormat.colorSpace,
        .imageExtent      = VkExtent2D{(extent.x), (extent.y)},
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform     = swapchain_support.capabilities.currentTransform,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode      = presentMode,
        .clipped          = VK_TRUE,
        .oldSwapchain     = VK_NULL_HANDLE,
    };

    const uint32_t graphic              = device_ptr->get_queues().get_queue(QueueSpecialization::Graphic)->index();
    const uint32_t present              = device_ptr->get_queues().get_queue(QueueSpecialization::Present)->index();
    uint32_t       queueFamilyIndices[] = {graphic, present};

    if (graphic != present)
    {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    VK_CHECK(vkCreateSwapchainKHR(device_ptr->raw(), &createInfo, nullptr, &ptr), "Failed to create swapchain")
    device_ptr->debug_set_object_name(get_definition().render_pass_ref.to_string(), ptr);

    vkGetSwapchainImagesKHR(device_ptr->raw(), ptr, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device_ptr->raw(), ptr, &imageCount, swapChainImages.data());

    image_available_semaphores.clear();
    for (uint32_t i = 0; i < device_ptr->get_image_count(); ++i)
        image_available_semaphores.emplace_back(Semaphore::create(get_definition().render_pass_ref.to_string() + "_sem_#" + std::to_string(i), device));

    if (get_definition().resize_callback_ptr)
        LOG_FATAL("Resize callback can't be used with swapchain draw pass");
    create_or_resize(extent, extent, true);
}

void Swapchain::draw()
{
    if (!render_internal())
        return;

    create_or_recreate();
    if (render_internal())
        LOG_ERROR("Failed to draw frame : {}x{}", extent.x, extent.y);
}

std::shared_ptr<ImageView> Swapchain::create_view_for_attachment(const std::string& name)
{
    return ImageView::create(name + "view", device, swapChainImages, ImageView::CreateInfos{.format = swapchain_format});
}

uint8_t Swapchain::get_framebuffer_count() const
{
    return device.lock()->get_image_count() + 1;
}

bool Swapchain::render_internal()
{
    PROFILER_SCOPE_NAMED(RenderPass_Draw, std::format("Draw swapchain"));
    reset_for_next_frame();
    const auto device_ref    = device.lock();
    uint8_t    current_frame = device.lock()->get_current_image();

    if (current_frame < in_flight_fences.size() && in_flight_fences[current_frame])
    {
        PROFILER_SCOPE(WaitSwapchainImageAvailable);
        in_flight_fences[current_frame]->wait();
    }
    device.lock()->flush_resources();

    uint32_t       image_index;
    const VkResult acquire_result = vkAcquireNextImageKHR(device_ref->raw(), ptr, UINT64_MAX, image_available_semaphores[current_frame]->raw(), VK_NULL_HANDLE, &image_index);
    if (acquire_result != VK_SUCCESS)
    {
        if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR || acquire_result == VK_SUBOPTIMAL_KHR)
            return true;
        VK_CHECK(acquire_result, "Failed to acquire next swapchain image")
    }

    if (current_frame >= in_flight_fences.size())
        in_flight_fences.resize(current_frame + 1, nullptr);
    in_flight_fences[current_frame] = get_render_finished_fence(static_cast<DeviceImageId>(image_index));

    RenderPassInstance::render(static_cast<uint8_t>(image_index), current_frame);

    // Submit to present queue
    const auto             render_finished_semaphore = framebuffers[image_index]->render_finished_semaphore().raw();
    const VkPresentInfoKHR present_infos{
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &render_finished_semaphore,
        .swapchainCount     = 1,
        .pSwapchains        = &ptr,
        .pImageIndices      = &image_index,
        .pResults           = nullptr,
    };

    const VkResult submit_result = device_ref->get_queues().get_queue(QueueSpecialization::Present)->present(present_infos);
    if (submit_result == VK_ERROR_OUT_OF_DATE_KHR || submit_result == VK_SUBOPTIMAL_KHR)
        return true;
    if (submit_result != VK_SUCCESS)
        VK_CHECK(acquire_result, "Failed to present swapchain image")

    return false;
}

void Swapchain::destroy()
{
    device.lock()->wait();
    in_flight_fences.clear();
    image_view = nullptr;

    if (ptr != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device.lock()->raw(), ptr, nullptr);
        ptr = VK_NULL_HANDLE;
    }
}

std::weak_ptr<ImageView> Swapchain::get_image_view() const
{
    return image_view;
}
} // namespace Eng::Gfx