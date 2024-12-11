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
            val.second->fill(desc_sets, ptr, found->second);
        }
    }
    vkUpdateDescriptorSets(device().lock()->raw(), static_cast<uint32_t>(desc_sets.size()), desc_sets.data(), 0, nullptr);
    outdated = false;
}

void DescriptorSet::bind_images(const std::string& binding_name, const std::vector<std::shared_ptr<ImageView>>& in_images)
{
    for (const auto& image : in_images)
        if (b_static && image->raw().size() > 1)
            LOG_ERROR("Cannot bind dynamic image '{}' to static descriptors '{}::{}'", image->get_name(), name, binding_name);
    try_insert(binding_name, std::make_shared<ImagesDescriptor>(in_images));
}

void DescriptorSet::bind_samplers(const std::string& binding_name, const std::vector<std::shared_ptr<Sampler>>& in_samplers)
{
    try_insert(binding_name, std::make_shared<SamplerDescriptor>(in_samplers));
}

void DescriptorSet::bind_buffers(const std::string& binding_name, const std::vector<std::shared_ptr<Buffer>>& in_buffers)
{
    for (const auto& buffer : in_buffers)
        if (b_static && buffer->raw().size() > 1)
            LOG_ERROR("Cannot bind dynamic buffer '{}' to static descriptors '{}::{}'", buffer->get_name(), name, binding_name);
    try_insert(binding_name, std::make_shared<BufferDescriptor>(in_buffers));
}

void DescriptorSet::ImagesDescriptor::fill(std::vector<VkWriteDescriptorSet>& out_sets, VkDescriptorSet dst_set, uint32_t binding)
{
    if (images.empty())
        LOG_FATAL("Empty image array descriptor");

    for (uint32_t i = 0; i < images.size(); ++i)
    {
        if (!images[i])
            LOG_FATAL("Invalid image descriptor");

        out_sets.emplace_back(VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = dst_set,
            .dstBinding      = binding,
            .dstArrayElement = i,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo = &images[i]->get_descriptor_infos_current(),
        });
    }
}

bool DescriptorSet::ImagesDescriptor::equals(const Descriptor& other) const
{
    const auto* other_ptr = static_cast<const ImagesDescriptor*>(&other);
    if (images.size() != other_ptr->images.size())
        return false;
    for (size_t i = 0; i < images.size(); ++i)
        if (images[i] != other_ptr->images[i])
            return false;
    return true;

}

void DescriptorSet::SamplerDescriptor::fill(std::vector<VkWriteDescriptorSet>& out_sets, VkDescriptorSet dst_set, uint32_t binding)
{
    if (samplers.empty())
        LOG_FATAL("Empty sampler array descriptor");

    for (uint32_t i = 0; i < samplers.size(); ++i)
    {
        if (!samplers[i])
            LOG_FATAL("Invalid sampler descriptor");

        out_sets.emplace_back(VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = dst_set,
            .dstBinding      = binding,
            .dstArrayElement = i,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .pImageInfo = &samplers[i]->get_descriptor_infos(),
        });
    }
}

bool DescriptorSet::SamplerDescriptor::equals(const Descriptor& other) const
{
    const auto* other_ptr = static_cast<const SamplerDescriptor*>(&other);
    if (samplers.size() != other_ptr->samplers.size())
        return false;
    for (size_t i = 0; i < samplers.size(); ++i)
        if (samplers[i] != other_ptr->samplers[i])
            return false;
    return true;
}

void DescriptorSet::BufferDescriptor::fill(std::vector<VkWriteDescriptorSet>& out_sets, VkDescriptorSet dst_set, uint32_t binding)
{
    if (buffers.empty())
        LOG_FATAL("Empty buffer array descriptor");

    for (uint32_t i = 0; i < buffers.size(); ++i)
    {
        if (!buffers[i])
            LOG_FATAL("Invalid buffer descriptor");

        out_sets.emplace_back(VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = dst_set,
            .dstBinding      = binding,
            .dstArrayElement = i,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &buffers[i]->get_descriptor_infos_current(),
        });
    }
}

bool DescriptorSet::BufferDescriptor::equals(const Descriptor& other) const
{
    const auto* other_ptr = static_cast<const BufferDescriptor*>(&other);
    if (buffers.size() != other_ptr->buffers.size())
        return false;
    for (size_t i = 0; i < buffers.size(); ++i)
        if (buffers[i] != other_ptr->buffers[i])
            return false;
    return true;
}

bool DescriptorSet::try_insert(const std::string& binding_name, const std::shared_ptr<Descriptor>& descriptor)
{
    std::lock_guard lk(update_lock);
    if (auto existing = write_descriptors.find(binding_name); existing != write_descriptors.end())
    {
        if (*descriptor == *existing->second)
            return false;
        existing->second = descriptor;
    }
    else
        write_descriptors.emplace(binding_name, descriptor);
    for (const auto& resource : resources)
        resource->mark_as_dirty();
    return true;
}
} // namespace Eng::Gfx