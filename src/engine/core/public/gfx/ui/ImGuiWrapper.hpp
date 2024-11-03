#pragma once
#include <memory>


struct ImGuiContext;

namespace Engine
{
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

		void draw(const CommandBuffer& cmd);

	private:
		std::shared_ptr<Mesh> mesh;
		std::weak_ptr<Device> device;
		std::shared_ptr<Pipeline> pipeline;
		std::shared_ptr<Image> font_texture;
		std::shared_ptr<ImageView> font_texture_view;
		ImGuiContext* imgui_context = nullptr;

	};
}
