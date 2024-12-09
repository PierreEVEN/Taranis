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
DescriptorSet::DescriptorSet(const std::string& in_name, const std::weak_ptr<Device>& in_device, const std::shared_ptr<Pipeline>& in_pipeline, bool b_in_static) : device(in_device), b_static(b_in_static), name(in_name)
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
    const auto descriptors = std::shared_ptr<DescriptorSet>(new DescriptorSet(name, device, pipeline, b_static));

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
        return resources[0]->raw();
    }
    auto& resource = resources[device.lock()->get_current_image()];
    resource->update();
    return resource->raw();
}

void DescriptorSet::Resource::update()
{
    if (!outdated)
        return;

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
    if (b_static && in_image->raw().size() > 1)
        LOG_ERROR("Cannot bind dynamic image '{}' to static descriptors '{}::{}'", in_image->get_name(), name, binding_name);

    //@TODO : improve this check by recreating new descriptors when needed
    std::lock_guard lk(update_lock);
    auto            new_descriptor = std::make_shared<ImageDescriptor>(in_image);
    if (auto existing = write_descriptors.find(binding_name); existing != write_descriptors.end())
    {
        if (*new_descriptor == *existing->second)
            return;
        existing->second = new_descriptor;
    }
    else
        write_descriptors.emplace(binding_name, new_descriptor);
    for (const auto& resource : resources)
        resource->mark_as_dirty();
}

void DescriptorSet::bind_sampler(const std::string& binding_name, const std::shared_ptr<Sampler>& in_sampler)
{
    std::lock_guard lk(update_lock);
    auto            new_descriptor = std::make_shared<SamplerDescriptor>(in_sampler);
    if (auto existing = write_descriptors.find(binding_name); existing != write_descriptors.end())
    {
        if (*new_descriptor == *existing->second)
            return;
        existing->second = new_descriptor;
    }
    else
        write_descriptors.emplace(binding_name, new_descriptor);
    for (const auto& resource : resources)
        resource->mark_as_dirty();
}

void DescriptorSet::bind_buffer(const std::string& binding_name, const std::shared_ptr<Buffer>& in_buffer)
{
    if (b_static && in_buffer->raw().size() > 1)
        LOG_ERROR("Cannot bind dynamic buffer '{}' to static descriptors '{}::{}'", in_buffer->get_name(), name, binding_name);

    if (name == "MaterialInstance__0_descriptors_shadows")
        LOG_DEBUG("bind buffer {} to {:x} : {}", (size_t)in_buffer.get(), size_t(this), name);
    std::lock_guard lk(update_lock);
    auto            new_descriptor = std::make_shared<BufferDescriptor>(in_buffer);
    if (auto existing = write_descriptors.find(binding_name); existing != write_descriptors.end())
    {
        if (*new_descriptor == *existing->second)
            return;
        existing->second = new_descriptor;
    }
    else
        write_descriptors.emplace(binding_name, new_descriptor);
    for (const auto& resource : resources)
        resource->mark_as_dirty();
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

void* DescriptorSet::ImageDescriptor::raw_ptr() const
{
    return image.get();
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

void* DescriptorSet::SamplerDescriptor::raw_ptr() const
{
    return sampler.get();
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

void* DescriptorSet::BufferDescriptor::raw_ptr() const
{
    return buffer.get();
}
} // namespace Eng::Gfx