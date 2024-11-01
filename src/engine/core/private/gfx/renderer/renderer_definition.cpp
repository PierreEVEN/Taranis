#include "gfx/renderer/renderer_definition.hpp"

#include "gfx/vulkan/swapchain.hpp"

namespace Engine
{
	std::shared_ptr<RendererStep> PresentStep::init_for_swapchain(const Swapchain& swapchain) const
	{
		auto renderer_step = RendererStep::create(pass_name, { Attachment::color("color", swapchain.get_format(), clear_value) });
		renderer_step->mark_as_present_pass();
		return renderer_step;
	}
}
