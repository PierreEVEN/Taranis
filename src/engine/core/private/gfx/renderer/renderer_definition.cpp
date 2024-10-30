#include "gfx/renderer/renderer_definition.hpp"

namespace Engine
{
	std::optional<ClearValue> Attachment::clear_value() const
	{
		return {};
	}
}
