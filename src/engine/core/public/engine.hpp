#pragma once
#include <memory>
#include <unordered_map>


namespace Engine
{
	class Config;
	class Device;
	class Instance;
}

namespace Engine
{
	class Window;
	struct WindowConfig;

	class Engine
	{
	public:
		Engine(Config& config);
		~Engine();

		std::weak_ptr<Window> new_window(const WindowConfig& config);

		void run();

		std::weak_ptr<Instance> get_instance() const { return gfx_instance; }
		std::weak_ptr<Device> get_device() const { return gfx_device; }

	private:
		std::unordered_map<size_t, std::shared_ptr<Window>> windows;

		std::shared_ptr<Instance> gfx_instance;
		std::shared_ptr<Device> gfx_device;

	};
}
