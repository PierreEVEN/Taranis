#include "gfx/vulkan/queue_family.hpp"

#include <ranges>
#include <unordered_map>

#include "gfx/vulkan/command_pool.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/instance.hpp"
#include "gfx/vulkan/physical_device.hpp"
#include "gfx/vulkan/surface.hpp"

namespace Engine::Gfx
{
Queues::Queues(const PhysicalDevice& physical_device, const Surface& surface)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device.raw(), &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device.raw(), &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;

    for (const auto& queueFamily : queueFamilies)
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device.raw(), i, surface.raw(), &presentSupport);
        auto queue = std::make_shared<QueueFamily>(i, queueFamily.queueFlags, presentSupport);
        all_queues.emplace_back(queue);
        i++;
    }

    update_specializations();
}

std::shared_ptr<QueueFamily> Queues::get_queue(QueueSpecialization specialization) const
{
    auto val = preferred.find(static_cast<uint8_t>(specialization));
    return val != preferred.end() ? val->second : nullptr;
}

void Queues::update_specializations()
{
    std::unordered_map<uint32_t, std::shared_ptr<QueueFamily>> queue_map;
    for (size_t i = 0; i < all_queues.size(); ++i)
    {
        all_queues[i]->set_name("queue-#" + std::to_string(i));
        queue_map.emplace(all_queues[i]->index(), all_queues[i]);
    }

    const auto stored_map = queue_map;

    // Find fallback queues for transfer and compute
    std::shared_ptr<QueueFamily> compute_queue  = find_best_suited_queue_family(queue_map, VK_QUEUE_COMPUTE_BIT, false, {});
    std::shared_ptr<QueueFamily> transfer_queue = find_best_suited_queue_family(queue_map, VK_QUEUE_TRANSFER_BIT, false, {});

    // Find graphic queue (ideally with present capability which should always be the case)
    std::shared_ptr<QueueFamily> graphic_queue = find_best_suited_queue_family(queue_map, VK_QUEUE_GRAPHICS_BIT, false, {});

    // Find present queue that is not a dedicated compute queue ideally
    std::shared_ptr<QueueFamily> present_queue = find_best_suited_queue_family(queue_map, 0, true, {});

    if (graphic_queue)
    {
        std::unordered_map<uint32_t, std::shared_ptr<QueueFamily>> no_graphic_queue_map = queue_map;
        no_graphic_queue_map.erase(graphic_queue->index());
        if (compute_queue)
            no_graphic_queue_map.erase(compute_queue->index());
        if (auto found_present_queue = find_best_suited_queue_family(no_graphic_queue_map, 0, true, {}))
            present_queue = found_present_queue;
        else
        {
            no_graphic_queue_map = queue_map;
            no_graphic_queue_map.erase(graphic_queue->index());
            if (found_present_queue = find_best_suited_queue_family(no_graphic_queue_map, 0, true, {}))
                present_queue = found_present_queue;
        }
    }

    if (graphic_queue)
        queue_map.erase(graphic_queue->index());
    if (present_queue)
        queue_map.erase(present_queue->index());

    // Search dedicated async compute queue
    std::shared_ptr<QueueFamily> async_compute_queue = find_best_suited_queue_family(queue_map, VK_QUEUE_COMPUTE_BIT, false, {});
    if (async_compute_queue)
        queue_map.erase(async_compute_queue->index());

    // Search dedicated transfer queue
    if (auto dedicated_transfer_queue = find_best_suited_queue_family(queue_map, VK_QUEUE_TRANSFER_BIT, false, {}))
    {
        queue_map.erase(dedicated_transfer_queue->index());
        transfer_queue = dedicated_transfer_queue;
    }

    preferred.clear();
    if (compute_queue)
    {
        graphic_queue->set_name("compute_queue");
        preferred.emplace(static_cast<uint8_t>(QueueSpecialization::Compute), compute_queue);
    }
    if (transfer_queue)
    {
        graphic_queue->set_name("transfer_queue");
        preferred.emplace(static_cast<uint8_t>(QueueSpecialization::Transfer), transfer_queue);
    }
    if (graphic_queue)
    {
        graphic_queue->set_name("graphic_queue");
        preferred.emplace(static_cast<uint8_t>(QueueSpecialization::Graphic), graphic_queue);
    }
    if (async_compute_queue)
    {
        graphic_queue->set_name("async_compute_queue");
        preferred.emplace(static_cast<uint8_t>(QueueSpecialization::AsyncCompute), async_compute_queue);
    }
    if (present_queue)
    {
        graphic_queue->set_name("present_queue");
        preferred.emplace(static_cast<uint8_t>(QueueSpecialization::Present), present_queue);
    }
}

const char* get_queue_specialization_name(QueueSpecialization elem)
{
    switch (elem)
    {
    case QueueSpecialization::Graphic:
        return "Graphic";
    case QueueSpecialization::Present:
        return "Present";
    case QueueSpecialization::Transfer:
        return "Transfer";
    case QueueSpecialization::Compute:
        return "Compute";
    case QueueSpecialization::AsyncCompute:
        return "AsyncCompute";
    }

    return "Unhandled queue specialization name";
}

QueueFamily::QueueFamily(uint32_t index, VkQueueFlags flags, bool support_present) : queue_index(index), queue_flags(flags), queue_support_present(support_present)
{
}

void QueueFamily::init_queue(const std::weak_ptr<Device>& device)
{
    vkGetDeviceQueue(device.lock()->raw(), index(), 0, &ptr);
    command_pool = CommandPool::create(name + "_cmd_pool", device, index());
    device.lock()->debug_set_object_name(name, ptr);
}

VkResult QueueFamily::present(const VkPresentInfoKHR& present_infos) const
{
    assert(queue_support_present);
    return vkQueuePresentKHR(ptr, &present_infos);
}

auto Queues::find_best_suited_queue_family(const std::unordered_map<uint32_t, std::shared_ptr<QueueFamily>>& available, VkQueueFlags required_flags, bool require_present,
                                           const std::vector<VkQueueFlags>& desired_queue_flags) -> std::shared_ptr<QueueFamily>
{
    uint32_t                     high_score = 0;
    std::shared_ptr<QueueFamily> best_queue;
    for (const auto& family : available | std::views::values)
    {
        if (require_present && !family->support_present())
            continue;
        if (required_flags && !(family->flags() & required_flags))
            continue;
        uint32_t score     = 0;
        best_queue         = family;
        uint32_t max_value = static_cast<uint32_t>(desired_queue_flags.size());
        for (int power = 0; power < desired_queue_flags.size(); ++power)
            if (family->flags() & desired_queue_flags[power])
                score += max_value - power;
        if (score > high_score)
        {
            high_score = score;
            best_queue = family;
        }
    }
    return best_queue;
}
} // namespace Engine::Gfx
