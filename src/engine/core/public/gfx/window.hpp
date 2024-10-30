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

		size_t get_id() const { return id; }

		bool render();

		void close()
		{
			should_close = true;
		}

	private:
		size_t id;
		bool should_close = false;
		GLFWwindow* ptr;
	};
}
