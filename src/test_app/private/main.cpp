
#include "config.hpp"
#include "engine.hpp"
#include <gfx/window.hpp>
#include <GLFW/glfw3.h>

int main() {

	Engine::Config config = {};

	Engine::Engine engine(config);


	const auto main_window = engine.new_window(Engine::WindowConfig{});


	while (!glfwWindowShouldClose(main_window->raw())) {
		glfwPollEvents();
	}
}
