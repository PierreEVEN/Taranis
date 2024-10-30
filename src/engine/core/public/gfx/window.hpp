#pragma once
#include <memory>
#include <string>
#include <glm/vec2.hpp>

struct GLFWwindow;

namespace Engine
{
	class RenderPass;

	struct WindowConfig
	{
		std::string name = "no name";
		glm::vec2 resolution = {800, 600};
	};


	class Window : public std::enable_shared_from_this<Window>
	{
	public:
		Window(const WindowConfig& config);
		~Window();

		GLFWwindow* raw() const { return ptr; }

		size_t get_id() const { return id; }

		bool render();

		glm::ivec2 internal_extent() const;

		void close()
		{
			should_close = true;
		}

		void set_renderer(std::shared_ptr<RenderPass> present_pass);

	private:

		const	
		size_t id;
		bool should_close = false;
		GLFWwindow* ptr;
	};
}
