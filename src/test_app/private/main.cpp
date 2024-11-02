#include "config.hpp"
#include "engine.hpp"
#include <gfx/window.hpp>

#include "gfx/shaders/shader_compiler.hpp"
#include "gfx/vulkan/device.hpp"

int main()
{
	Logger::get().enable_logs(
		Logger::LOG_LEVEL_DEBUG | Logger::LOG_LEVEL_ERROR | Logger::LOG_LEVEL_FATAL | Logger::LOG_LEVEL_INFO |
		Logger::LOG_LEVEL_WARNING | Logger::LOG_LEVEL_TRACE);

	Engine::ShaderCompiler cmp;

	if (auto res = cmp.load_from_path("./resources/test.hlsl", "VSMain", Engine::EShaderStage::Vertex, true))
	{

		LOG_INFO("PC size : {}", res.get().push_constant_size);

		for (const auto& binding : res.get().bindings)
			LOG_INFO("\t Binding : {} ({}) => {}", binding.name, binding.binding, magic_enum::enum_name(binding.type).data());
		
	} else
	{
		LOG_FATAL("{}", res.error());
	}

	LOG_INFO("DONE !");


	exit(0);

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
