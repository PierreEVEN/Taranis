#include "gfx/vulkan/descriptor_sets.hpp"

#include "profiler.hpp"

#include <ranges>
#include <utility>

#include "gfx/vulkan/descriptor_pool.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/pipeline.hpp"
#include "gfx/vulkan/sampler.hpp"
#include "shader_compiler/shader_compiler.hpp"

namespace Eng::Gfx
{
DescriptorSet::DescriptorSet(const std::weak_ptr<Device>& in_device, const std::shared_ptr<Pipeline>& in_pipeline, bool b_in_static) : device(in_device), b_static(b_in_static)
{
    for (const auto& binding : in_pipeline->get_bindings())
        descriptor_bindings.insert_or_assign(binding.name, binding.binding);
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
    std::lock_guard lk(update_lock);
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
    update_count++;
    auto test_name = std::format("{:x}", size_t(ptr));
    if (test_name == "319350000005fed" || test_name == "9f68c90000008e77" || test_name == "9b3b5a0000008e74" || test_name == "d9af760000008e78" || test_name == "cba2760000008e83" || test_name == "c760ac0000008e72")
    {
        LOG_DEBUG("GREUGNEUH {:x}", (size_t)ptr);
    }

    if (update_count > 1)
        LOG_DEBUG("Reupdate {:x}", (size_t)ptr);

    PROFILER_SCOPE(UpdateDescriptorSets);
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
    outdated = false;
}

void DescriptorSet::bind_image(const std::string& binding_name, const std::shared_ptr<ImageView>& in_image)
{
    std::lock_guard lk(update_lock);
    if (!in_image)
        LOG_FATAL("Cannot set null image in descriptor {}", binding_name);
    for (const auto& resource : resources)
        resource->outdated = true;
    write_descriptors.insert_or_assign(binding_name, std::make_shared<ImageDescriptor>(in_image));
}

void DescriptorSet::bind_sampler(const std::string& binding_name, const std::shared_ptr<Sampler>& in_sampler)
{
    std::lock_guard lk(update_lock);
    for (const auto& resource : resources)
        resource->outdated = true;
    write_descriptors.insert_or_assign(binding_name, std::make_shared<SamplerDescriptor>(in_sampler));
}

void DescriptorSet::bind_buffer(const std::string& binding_name, const std::shared_ptr<Buffer>& in_buffer)
{
    std::lock_guard lk(update_lock);
    for (const auto& resource : resources)
        resource->outdated = true;
    write_descriptors.insert_or_assign(binding_name, std::make_shared<BufferDescriptor>(in_buffer));
}

VkWriteDescriptorSet DescriptorSet::ImageDescriptor::get()
{
    if (!image)
        LOG_FATAL("Invalid image descriptor");
    return VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo = &image->get_descriptor_infos_current(),
    };
}

VkWriteDescriptorSet DescriptorSet::SamplerDescriptor::get()
{
    if (!sampler)
        LOG_FATAL("Invalid sampler descriptor");
    return VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo = &sampler->get_descriptor_infos(),
    };
}

VkWriteDescriptorSet DescriptorSet::BufferDescriptor::get()
{
    if (!buffer)
        LOG_FATAL("Invalid buffer descriptor");
    return VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &buffer->get_descriptor_infos_current(),
    };
}
} // namespace Eng::Gfx