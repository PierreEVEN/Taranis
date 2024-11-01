#include "config.hpp"
#include "engine.hpp"
#include <gfx/window.hpp>

#include "gfx/vulkan/device.hpp"

int main()
{
	Engine::Config config = {};

	Engine::Engine engine(config);

	const auto main_window = engine.new_window(Engine::WindowConfig{});
	main_window.lock()->set_renderer(
		Engine::PresentStep::create("present_pass", Engine::ClearValue::color({ 1, 0, 0 ,1 }))
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
