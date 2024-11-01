#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

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

		struct VertexInput
		{
			uint32_t location;
			uint32_t offset;
			ColorFormat format;
		};

		Pipeline(std::weak_ptr<Device> device, std::shared_ptr<RenderPassObject> render_pass, std::vector<VertexInput> vertex_inputs, std::vector<std::shared_ptr<ShaderModule>> shader_stage, const CreateInfos& create_infos);
		~Pipeline();

	private:
		VkPipelineLayout layout = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
		VkPipeline ptr = VK_NULL_HANDLE;
		std::weak_ptr<Device> device;
	};
}
