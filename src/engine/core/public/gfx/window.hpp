#pragma once
#include <string>
#include <glm/vec2.hpp>

struct GLFWwindow;

namespace Engine
{
	struct WindowConfig
	{
		std::string name = "no name";
		glm::vec2 resolution = {800, 600};
	};


	class Window
	{
	public:
		Window(const WindowConfig& config);
		~Window();

		GLFWwindow* raw() const { return ptr; }

		bool render();

		void close()
		{
			should_close = true;
		}

	private:

		bool should_close;
		GLFWwindow* ptr;
	};
}
