#pragma once
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Engine
{
	class Window;
	using RenderPassName = std::string;

	class ClearValue
	{
	};

	enum class AttachmentTargetType
	{
		InternalColor,
		InternalDepth,
		Window
	};

	class Attachment
	{
	public:
		VkFormat get_format() const { return format; }
		bool is_present_pass() const { return target_type == AttachmentTargetType::Window; }
		std::optional<ClearValue> clear_value() const;
		bool is_depth() const { return target_type == AttachmentTargetType::InternalDepth; }

		static Attachment depth(std::string name, VkFormat format)
		{
			Attachment attachment(AttachmentTargetType::InternalDepth, std::move(name));
			attachment.format = format;
			return attachment;
		}

		static Attachment color(std::string name, VkFormat format)
		{
			Attachment attachment(AttachmentTargetType::InternalColor, std::move(name));
			attachment.format = format;
			return attachment;
		}

		static Attachment window(std::string name)
		{
			Attachment attachment(AttachmentTargetType::Window, std::move(name));
			return attachment;
		}

		bool operator==(const Attachment& other) const
		{
			return get_format() == other.get_format() && target_type == other.target_type;
		}

	private:
		Attachment(AttachmentTargetType in_target_type, std::string in_name) : name(std::move(in_name)), target_type(
			                                                                       in_target_type)
		{
		}

		std::string name;
		AttachmentTargetType target_type;
		VkFormat format = VK_FORMAT_UNDEFINED;
	};

	class RenderPassInfos
	{
	public:
		bool operator==(const RenderPassInfos& other) const
		{
			return attachments == other.attachments;
		}

		std::vector<Attachment> attachments;
	};

	class RenderPass : public std::enable_shared_from_this<RenderPass>
	{
	public:
		static std::shared_ptr<RenderPass> create(RenderPassName name, std::vector<Attachment> in_attachments)
		{
			return std::shared_ptr<RenderPass>(new RenderPass(std::move(name), std::move(in_attachments)));
		}

		std::shared_ptr<RenderPass> attach(std::shared_ptr<RenderPass> dependency)
		{
			dependencies.emplace(std::move(dependency));
			return shared_from_this();
		}

		const RenderPassInfos& get_infos() const { return infos; }

	private:
		std::string pass_name;
		RenderPassInfos infos;
		std::unordered_set<std::shared_ptr<RenderPass>> dependencies;

		RenderPass(RenderPassName name, std::vector<Attachment> in_attachments) : pass_name(std::move(name))
		{
			infos.attachments = std::move(in_attachments);
		}
	};
}

template <>
struct std::hash<Engine::RenderPassInfos>
{
	size_t operator()(const Engine::RenderPassInfos& val) const noexcept
	{
		auto ite = val.attachments.begin();
		if (ite == val.attachments.end())
			return 0;
		size_t hash = std::hash<int32_t>()(ite->get_format() + 1);
		for (; ite != val.attachments.end(); ++ite)
		{
			hash ^= std::hash<int32_t>()(ite->get_format() + 1) * 13;
		}
		return hash;
	}
};
