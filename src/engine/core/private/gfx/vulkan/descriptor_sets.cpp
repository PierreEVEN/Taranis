#include "gfx/vulkan/descriptor_sets.hpp"

#include <ranges>
#include <utility>

#include "gfx/vulkan/descriptor_pool.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/pipeline.hpp"
#include "gfx/vulkan/sampler.hpp"

namespace Engine
{
DescriptorSet::DescriptorSet(const std::weak_ptr<Device>& in_device, const std::shared_ptr<Pipeline>& in_pipeline, bool b_in_static) : device(in_device), b_static(b_in_static)
{
    for (const auto& binding : in_pipeline->get_bindings())
        descriptor_bindings.emplace(binding.name, binding.binding);
}

DescriptorSet::Resource::Resource(const std::string& name, const std::weak_ptr<Device>& in_device, const std::weak_ptr<DescriptorSet>& in_parent, const std::shared_ptr<Pipeline>& in_pipeline)
    : DeviceResource(in_device), pipeline(in_pipeline), parent(in_parent.lock())
{
    ptr = device().lock()->get_descriptor_pool().allocate(*pipeline, pool_index);
    device().lock()->debug_set_object_name(name, ptr);
}

DescriptorSet::Resource::~Resource()
{
    device().lock()->get_descriptor_pool().free(ptr, *pipeline, pool_index);
}

DescriptorSet::~DescriptorSet()
{
    for (const auto& desc_set : resources)
    {
        device.lock()->drop_resource(desc_set);
    }
}

std::shared_ptr<DescriptorSet> DescriptorSet::create(const std::string& name, const std::weak_ptr<Device>& device, const std::shared_ptr<Pipeline>& pipeline, bool b_static)
{
    const auto descriptors = std::shared_ptr<DescriptorSet>(new DescriptorSet(device, pipeline, b_static));

    if (b_static)
        descriptors->resources = std::vector{std::make_shared<Resource>(name, device, descriptors, pipeline)};
    else
        for (size_t i = 0; i < device.lock()->get_image_count(); ++i)
            descriptors->resources.push_back(std::make_shared<Resource>(name + "_#" + std::to_string(i), device, descriptors, pipeline));

    return descriptors;
}

const VkDescriptorSet& DescriptorSet::raw_current() const
{
    if (b_static)
    {
        resources[0]->update();
        return resources[0]->ptr;
    }
    auto& resource = resources[device.lock()->get_current_image()];
    resource->update();
    return resource->ptr;
}

void DescriptorSet::Resource::update()
{
    if (!outdated)
        return;
    outdated = false;
    std::vector<VkWriteDescriptorSet> desc_sets;
    auto                              parent_ptr = parent.lock();
    for (const auto& val : parent_ptr->write_descriptors)
    {
        if (auto found = parent_ptr->descriptor_bindings.find(val.first); found != parent_ptr->descriptor_bindings.end())
        {
            auto desc_set       = val.second->get();
            desc_set.dstSet     = ptr;
            desc_set.dstBinding = found->second;
            desc_sets.emplace_back(desc_set);
        }
    }
    vkUpdateDescriptorSets(device().lock()->raw(), static_cast<uint32_t>(desc_sets.size()), desc_sets.data(), 0, nullptr);
}

void DescriptorSet::bind_image(const std::string& binding_name, const std::shared_ptr<ImageView>& in_image)
{
    for (const auto& resource : resources)
        resource->outdated = true;
    write_descriptors.emplace(binding_name, std::make_shared<ImageDescriptor>(in_image));
}

void DescriptorSet::bind_sampler(const std::string& binding_name, const std::shared_ptr<Sampler>& in_sampler)
{
    for (const auto& resource : resources)
        resource->outdated = true;
    write_descriptors.emplace(binding_name, std::make_shared<SamplerDescriptor>(in_sampler));
}

VkWriteDescriptorSet DescriptorSet::ImageDescriptor::get()
{
    return VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo = &image->get_descriptor_infos_current(),
    };
}

VkWriteDescriptorSet DescriptorSet::SamplerDescriptor::get()
{
    return VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo = &sampler->get_descriptor_infos(),
    };
}
} // namespace Engine