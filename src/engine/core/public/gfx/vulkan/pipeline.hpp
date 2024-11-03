#pragma once
#include <memory>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "gfx/shaders/shader_compiler.hpp"

namespace Engine
{
	enum class ColorFormat;
}

namespace Engine
{
	class ShaderModule;
	class RenderPassObject;
	class Device;
}

namespace Engine
{
	enum class ECulling
	{
		None,
		Front,
		Back,
		Both
	};

	enum class EFrontFace
	{
		Clockwise,
		CounterClockwise,
	};

	enum class ETopology
	{
		Points,
		Lines,
		Triangles,
	};

	enum class EPolygonMode
	{
		Point,
		Line,
		Fill,
	};

	enum class EAlphaMode
	{
		Opaque,
		Translucent,
		Additive
	};

	class Pipeline
	{
	public:
		struct CreateInfos
		{
			ECulling culling = ECulling::Back;
			EFrontFace front_face = EFrontFace::CounterClockwise;
			ETopology topology = ETopology::Triangles;
			EPolygonMode polygon_mode = EPolygonMode::Fill;
			EAlphaMode alpha_mode = EAlphaMode::Opaque;
			bool depth_test = true;
			float line_width = 1.0f;
		};

		Pipeline(std::weak_ptr<Device> device, std::weak_ptr<RenderPassObject> render_pass, std::vector<std::shared_ptr<ShaderModule>> shader_stage, const CreateInfos& create_infos);
		~Pipeline();

		const VkPipelineLayout& get_layout() const { return layout; }
		const VkDescriptorSetLayout& get_descriptor_layout() const { return descriptor_set_layout; }
		const VkPipeline& raw() const { return ptr; }
		const CreateInfos& infos() const { return create_infos; }
		const std::vector<BindingDescription>& get_bindings() const { return descriptor_bindings; }

	private:
		std::vector<BindingDescription> descriptor_bindings;
		CreateInfos create_infos;
		VkPipelineLayout layout = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
		VkPipeline ptr = VK_NULL_HANDLE;
		std::weak_ptr<Device> device;
	};
}
