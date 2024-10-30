#pragma once
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Engine
{
	class ClearValue
	{
		
	};

	class Attachment
	{
	public:
		VkFormat get_format() const { return format; }
		bool is_present_pass() const { return present_pass; }
		std::optional<ClearValue> clear_value() const;
	private:
		bool present_pass = false;
		VkFormat format = VK_FORMAT_UNDEFINED;
	};


	class RenderPassInfos
	{
	public:
		std::vector<Attachment> color_attachments;
		std::optional<Attachment> depth_attachments;
	};
}
