#include "config.hpp"
#include "engine.hpp"
#include <gfx/window.hpp>

#include "gfx/renderer/renderer.hpp"
#include "gfx/vulkan/device.hpp"

int main()
{
	Engine::Config config = {};

	Engine::Engine engine(config);

	engine.get_device().lock()->declare_render_pass(RenderpassCre)


	const auto main_renderer = Engine::RenderPass::create("present_pass")
		->attach(Engine::RenderPass::create("forward_pass"))
		->attach(Engine::RenderPass::create("forward_depth"));


	const auto main_window = engine.new_window(Engine::WindowConfig{});

	main_window.lock()->set_renderer(main_renderer);

	const auto secondary_window = engine.new_window(Engine::WindowConfig{});

	engine.run();
}
