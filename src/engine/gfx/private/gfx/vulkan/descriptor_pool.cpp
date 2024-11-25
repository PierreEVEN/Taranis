#include "gfx/vulkan/descriptor_pool.hpp"

#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/pipeline.hpp"
#include "gfx/vulkan/vk_check.hpp"
#include "shader_compiler/shader_compiler.hpp"

static constexpr uint32_t DESCRIPTOR_PER_POOL = 200;

namespace Eng::Gfx
{
DescriptorPool::DescriptorPool(std::weak_ptr<Device> in_device) : device(std::move(in_device))
{
}

PoolDescription::PoolDescription(const Pipeline& pipeline)
{
    std::unordered_map<VkDescriptorType, uint32_t> sizes;

    for (const auto& binding : pipeline.get_bindings())
    {
        VkDescriptorType type;
        switch (binding.type)
        {
        case EBindingType::SAMPLER:
            type = VK_DESCRIPTOR_TYPE_SAMPLER;
            break;
        case EBindingType::COMBINED_IMAGE_SAMPLER:
            type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            break;
        case EBindingType::SAMPLED_IMAGE:
            type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            break;
        case EBindingType::STORAGE_IMAGE:
            type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            break;
        case EBindingType::UNIFORM_TEXEL_BUFFER:
            type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            break;
        case EBindingType::STORAGE_TEXEL_BUFFER:
            type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            break;
        case EBindingType::UNIFORM_BUFFER:
            type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            break;
        case EBindingType::STORAGE_BUFFER:
            type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            break;
        case EBindingType::UNIFORM_BUFFER_DYNAMIC:
            type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            break;
        case EBindingType::STORAGE_BUFFER_DYNAMIC:
            type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            break;
        case EBindingType::INPUT_ATTACHMENT:
            type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            break;
        default:
            LOG_FATAL("Unhandled binding type")
        }

        auto found = sizes.find(type);
        if (found != sizes.end())
            found->second++;
        else
            sizes.emplace(type, 1);
    }

    for (const auto& entry : sizes)
        pool_sizes.emplace_back(entry.first, entry.second);
    std::ranges::sort(pool_sizes,
                      [](const VkDescriptorPoolSize& a, const VkDescriptorPoolSize& b)
                      {
                          return a.type > b.type;
                      });
}

VkDescriptorSet DescriptorPool::allocate(const Pipeline& pipeline, size_t& pool_index)
{
    PoolDescription description(pipeline);

    std::lock_guard lock(pool_lock);
    auto            found = pools.find(description);
    if (found == pools.end())
        found = pools.emplace(description, std::pair{0, std::vector<std::shared_ptr<Pool>>{}}).first;
    auto& found_pool = found->second;

    for (size_t i = found_pool.first; i < found_pool.second.size(); ++i)
    {
        if (const auto handle = found_pool.second[i]->allocate(pipeline.get_descriptor_layout()))
        {
            pool_index = i;
            return handle;
        }
    }
    const auto new_pool = std::make_shared<Pool>(device, description);
    pool_index          = found_pool.second.size();
    found_pool.first    = pool_index;
    const auto handle   = new_pool->allocate(pipeline.get_descriptor_layout());
    found_pool.second.push_back(new_pool);
    return handle;
}

void DescriptorPool::free(const VkDescriptorSet& desc_set, const Pipeline& pipeline, size_t pool_index)
{
    PoolDescription description(pipeline);

    std::lock_guard lock(pool_lock);
    auto            found = pools.find(description);
    if (found == pools.end())
    {
        LOG_FATAL("Cannot free descriptors : pool not found")
    }
    auto& found_pool = found->second;
    if (pool_index < found_pool.first)
        found_pool.first = pool_index;

    if (found_pool.second[pool_index]->free(desc_set))
    {
        while (!found_pool.second.empty() && found_pool.second.back()->is_empty())
            found_pool.second.pop_back();
        if (found_pool.second.empty())
            found_pool.first = 0;
        else if (found_pool.second.size() - 1 < found_pool.first)
            found_pool.first = found_pool.second.size() - 1;
    }
}

DescriptorPool::Pool::Pool(std::weak_ptr<Device> in_device, const PoolDescription& description) : initial_space(DESCRIPTOR_PER_POOL), device(std::move(in_device))
{
    auto pool_sizes = description.pool_sizes;
    for (auto& pool_size : pool_sizes)
        pool_size.descriptorCount *= DESCRIPTOR_PER_POOL;
    space_left = initial_space;
    const VkDescriptorPoolCreateInfo poolInfo{
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = static_cast<VkDescriptorPoolCreateFlags>(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT),
        .maxSets       = space_left,
        .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
        .pPoolSizes    = pool_sizes.data(),
    };

    VK_CHECK(vkCreateDescriptorPool(device.lock()->raw(), &poolInfo, nullptr, &ptr), "Failed to create descriptor pool")
}

DescriptorPool::Pool::~Pool()
{
    vkDestroyDescriptorPool(device.lock()->raw(), ptr, nullptr);
}

VkDescriptorSet DescriptorPool::Pool::allocate(const VkDescriptorSetLayout& layout)
{
    if (space_left == 0)
        return VK_NULL_HANDLE;

    space_left--;
    const VkDescriptorSetAllocateInfo descriptor_info{
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = ptr,
        .descriptorSetCount = 1,
        .pSetLayouts        = &layout,
    };
    VkDescriptorSet descriptor_set;
    VK_CHECK(vkAllocateDescriptorSets(device.lock()->raw(), &descriptor_info, &descriptor_set), "failed to allocate descriptor sets")
    return descriptor_set;
}

bool DescriptorPool::Pool::free(const VkDescriptorSet& desc_set)
{
    vkFreeDescriptorSets(device.lock()->raw(), ptr, 1, &desc_set);
    space_left++;
    return is_empty();
}
} // namespace Eng::Gfx
