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
	DescriptorSet::DescriptorSet(std::weak_ptr<Device> in_device, const std::shared_ptr<Pipeline>& in_pipeline) : pipeline(in_pipeline), device(std::move(
		in_device))
	{
		ptr = device.lock()->get_descriptor_pool().allocate(*pipeline, pool_index);

		for (const auto& binding : in_pipeline->get_bindings())
			descriptor_bindings.emplace(binding.name, binding.binding);
	}

	DescriptorSet::~DescriptorSet()
	{
		device.lock()->get_descriptor_pool().free(ptr, *pipeline, pool_index);
	}

	void DescriptorSet::update()
	{
		if (!outdated)
			return;
		outdated = false;
		std::vector<VkWriteDescriptorSet> desc_sets;
		for (const auto& val : write_descriptors)
		{
			if (auto found = descriptor_bindings.find(val.first); found != descriptor_bindings.end()) {
				auto desc_set = val.second->get();
				desc_set.dstSet = ptr;
				desc_set.dstBinding = found->second;
				desc_sets.emplace_back(desc_set);
			}
		}
		vkUpdateDescriptorSets(device.lock()->raw(), static_cast<uint32_t>(desc_sets.size()), desc_sets.data(), 0,
		                       nullptr);
	}

	void DescriptorSet::bind_image(const std::string& binding_name, const std::shared_ptr<ImageView>& in_image)
	{
		outdated = true;
		write_descriptors.emplace(binding_name, std::make_shared<ImageDescriptor>(in_image));
	}

	void DescriptorSet::bind_sampler(const std::string& binding_name, const std::shared_ptr<Sampler>& in_sampler)
	{
		outdated = true;
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
}
