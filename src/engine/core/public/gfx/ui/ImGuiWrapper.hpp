#pragma once
#include <memory>
#include <glm/vec2.hpp>


struct ImGuiContext;

namespace Engine
{
	class DescriptorSet;
	class Sampler;
	class ImageView;
	class Image;
	class CommandBuffer;
	class Mesh;
	class RenderPassObject;
	class Device;
	class Pipeline;
}

namespace Engine
{
	class ImGuiWrapper
	{
	public:
		ImGuiWrapper(const std::weak_ptr<RenderPassObject>& render_pass, std::weak_ptr<Device> device);

		void draw(const CommandBuffer& cmd, glm::uvec2 draw_res);

	private:
		std::shared_ptr<Mesh> mesh;
		std::shared_ptr<Pipeline> imgui_material;
		std::shared_ptr<DescriptorSet> imgui_descriptors;
		std::shared_ptr<Image> font_texture;
		std::shared_ptr<Sampler> image_sampler;
		std::shared_ptr<ImageView> font_texture_view;
		std::weak_ptr<Device> device;
		ImGuiContext* imgui_context = nullptr;

	};
}
