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
		Engine::RenderPass::create("present_pass", {
			                           Engine::Attachment::color("col", VK_FORMAT_R8G8B8A8_UNORM)
		                           })
		->attach(Engine::RenderPass::create("forward_pass", {
			                                    Engine::Attachment::color("color", VK_FORMAT_R8G8B8A8_UNORM),
			                                    Engine::Attachment::depth("depth", VK_FORMAT_D24_UNORM_S8_UINT)
		                                    }))
		->attach(Engine::RenderPass::create("forward_test", {
			                                    Engine::Attachment::color("color", VK_FORMAT_R8G8B8A8_UNORM),
			                                    Engine::Attachment::color("normal", VK_FORMAT_R8G8B8A8_UNORM),
			                                    Engine::Attachment::depth("depth", VK_FORMAT_D32_SFLOAT)
		                                    })));
	const auto secondary_window = engine.new_window(Engine::WindowConfig{});
	engine.run();
}
