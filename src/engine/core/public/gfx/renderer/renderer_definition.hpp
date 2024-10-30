#pragma once
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
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
		std::string name;
		std::vector<Attachment> color_attachments;
		std::optional<Attachment> depth_attachments;
	};

	class RenderPass : public std::enable_shared_from_this<RenderPass>
	{
	public:

		static std::shared_ptr<RenderPass> create(const std::string& name)
		{
			return std::make_shared<RenderPass>(name);
		}

		std::shared_ptr<RenderPass> attach(std::shared_ptr<RenderPass> dependency)
		{
			dependencies.emplace(std::move(dependency));
			return shared_from_this();
		}

	private:
		std::string pass_name;
		std::unordered_set<std::shared_ptr<RenderPass>> dependencies;
		RenderPass(std::string name) : pass_name(std::move(name))
		{
		}
	};
}
