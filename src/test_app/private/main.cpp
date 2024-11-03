#include "config.hpp"
#include "engine.hpp"
#include <gfx/window.hpp>

#include "gfx/renderer/renderer.hpp"
#include "gfx/shaders/shader_compiler.hpp"
#include "gfx/ui/ImGuiWrapper.hpp"
#include "gfx/vulkan/device.hpp"
#include "imgui.h"


namespace Engine
{
	class ImGuiWrapper;
}

class TestFirstPassInterface : public Engine::RenderPassInterface
{
public:
	void init(const std::weak_ptr<Engine::Device>& device, const Engine::RenderPassInstanceBase& render_pass) override
	{
		imgui = std::make_unique<Engine::ImGuiWrapper>(render_pass.get_render_pass(), device);
	}
	void render(const Engine::RenderPassInstanceBase&) override
	{

	}
private:
	std::unique_ptr<Engine::ImGuiWrapper> imgui;
};

int main()
{
	Logger::get().enable_logs(
		Logger::LOG_LEVEL_DEBUG | Logger::LOG_LEVEL_ERROR | Logger::LOG_LEVEL_FATAL | Logger::LOG_LEVEL_INFO |
		Logger::LOG_LEVEL_WARNING | Logger::LOG_LEVEL_TRACE);

	Engine::Config config = {};

	Engine::Engine engine(config);

	const auto main_window = engine.new_window(Engine::WindowConfig{});
	main_window.lock()->set_renderer(
		Engine::PresentStep::create<TestFirstPassInterface>("present_pass", Engine::ClearValue::color({ 1, 0, 0 ,1 }))
		->attach(Engine::RendererStep::create("forward_pass", {
			                                      Engine::Attachment::color(
				                                      "color", Engine::ColorFormat::R8G8B8A8_UNORM),
			                                      Engine::Attachment::depth(
				                                      "depth", Engine::ColorFormat::D24_UNORM_S8_UINT)
		                                      }))
		->attach(Engine::RendererStep::create("forward_test", {
			                                      Engine::Attachment::color(
				                                      "color", Engine::ColorFormat::R8G8B8A8_UNORM),
			                                      Engine::Attachment::color(
				                                      "normal", Engine::ColorFormat::R8G8B8A8_UNORM),
			                                      Engine::Attachment::depth("depth", Engine::ColorFormat::D32_SFLOAT)
		                                      })));
	const auto secondary_window = engine.new_window(Engine::WindowConfig{});
	engine.run();
}
