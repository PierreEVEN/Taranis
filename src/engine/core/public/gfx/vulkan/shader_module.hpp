#pragma once
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Engine
{
	class Device;

	enum class EShaderStage
	{
		Vertex = 0x00000001,
		Fragment = 0x00000010
	};

	enum class EBindingType
	{
		SAMPLER,
		COMBINED_IMAGE_SAMPLER,
		SAMPLED_IMAGE,
		STORAGE_IMAGE,
		UNIFORM_TEXEL_BUFFER,
		STORAGE_TEXEL_BUFFER,
		UNIFORM_BUFFER,
		STORAGE_BUFFER,
		UNIFORM_BUFFER_DYNAMIC,
		STORAGE_BUFFER_DYNAMIC,
		INPUT_ATTACHMENT
	};

	using Spirv = std::vector<uint32_t>;

	class ShaderModule
	{
	public:
		struct Bindings
		{
			Bindings(uint32_t in_binding, EBindingType in_type) : binding(in_binding), type(in_type)
			{
			}

			uint32_t binding;
			EBindingType type;
		};

		struct CreateInfos
		{
			EShaderStage stage = EShaderStage::Vertex;
			uint32_t push_constant_size = 0;
			std::string entry_point = "main";
		};

		ShaderModule(std::weak_ptr<Device> device, const Spirv& spirv, const std::vector<Bindings>& bindings,
		             const CreateInfos& create_infos);
		~ShaderModule();

		VkShaderModule raw() const { return ptr; }

		const CreateInfos& infos() const { return create_infos; }
		const std::vector<Bindings>& get_bindings() const { return bindings; };

	private:
		std::vector<Bindings> bindings;
		CreateInfos create_infos;
		VkShaderModule ptr = VK_NULL_HANDLE;
		std::weak_ptr<Device> device;
	};
}
