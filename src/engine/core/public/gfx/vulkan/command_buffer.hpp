#pragma once
#include <memory>

namespace Engine
{
	class Device;

	class CommandBuffer
	{
	public:
		CommandBuffer(std::weak_ptr<Device> device);
	};
}
