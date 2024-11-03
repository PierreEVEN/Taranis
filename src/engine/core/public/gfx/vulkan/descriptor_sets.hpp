#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan_core.h>


namespace Engine
{
	class ImageView;
	class Pipeline;
	class Sampler;
	class Device;

	class DescriptorSet
	{
	public:
		DescriptorSet(std::weak_ptr<Device> device, const std::shared_ptr<Pipeline>& pipeline);
		~DescriptorSet();

		const VkDescriptorSet& raw() const { return ptr; }

		void update();


		void bind_image(const std::string& binding_name, const std::shared_ptr<ImageView>& in_image);
		void bind_sampler(const std::string& binding_name, const std::shared_ptr<Sampler>& in_sampler);

	private:
		class Descriptor
		{
		public:
			virtual VkWriteDescriptorSet get() = 0;
		};

		class ImageDescriptor : public Descriptor
		{
		public:
			ImageDescriptor(std::shared_ptr<ImageView> in_image) : image(std::move(in_image))
			{
			}
			VkWriteDescriptorSet get() override;
			std::shared_ptr<ImageView> image;
		};

		class SamplerDescriptor : public Descriptor
		{
		public:
			SamplerDescriptor(std::shared_ptr<Sampler> in_sampler) : sampler(std::move(in_sampler))
			{
			}

			VkWriteDescriptorSet get() override;
			std::shared_ptr<Sampler> sampler;
		};

		std::unordered_map<std::string, std::shared_ptr<Descriptor>> write_descriptors;
		std::unordered_map<std::string, uint32_t> descriptor_bindings;

		bool outdated = false;
		size_t pool_index = 0;
		std::shared_ptr<Pipeline> pipeline;
		std::weak_ptr<Device> device;
		VkDescriptorSet ptr = VK_NULL_HANDLE;
	};
}
