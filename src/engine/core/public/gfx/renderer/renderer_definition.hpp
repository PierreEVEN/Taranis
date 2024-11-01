#pragma once
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "gfx/types.hpp"

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
		ColorFormat get_format() const { return format; }
		bool is_present_pass() const { return target_type == AttachmentTargetType::Window; }
		std::optional<ClearValue> clear_value() const;
		bool is_depth() const { return target_type == AttachmentTargetType::InternalDepth; }

		static Attachment depth(std::string name, ColorFormat format)
		{
			Attachment attachment(AttachmentTargetType::InternalDepth, std::move(name));
			attachment.format = format;
			return attachment;
		}

		static Attachment color(std::string name, ColorFormat format)
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
		ColorFormat format = ColorFormat::UNDEFINED;
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

	class RendererStep : public std::enable_shared_from_this<RendererStep>
	{
	public:
		static std::shared_ptr<RendererStep> create(RenderPassName name, std::vector<Attachment> in_attachments)
		{
			return std::shared_ptr<RendererStep>(new RendererStep(std::move(name), std::move(in_attachments)));
		}

		std::shared_ptr<RendererStep> attach(std::shared_ptr<RendererStep> dependency)
		{
			dependencies.emplace(std::move(dependency));
			return shared_from_this();
		}

		const RenderPassInfos& get_infos() const { return infos; }

		const std::unordered_set<std::shared_ptr<RendererStep>>& get_dependencies() const { return dependencies; }

	private:
		std::string pass_name;
		RenderPassInfos infos;
		std::unordered_set<std::shared_ptr<RendererStep>> dependencies;

		RendererStep(RenderPassName name, std::vector<Attachment> in_attachments) : pass_name(std::move(name))
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
		size_t hash = std::hash<int32_t>()(static_cast<uint32_t>(ite->get_format()) + 1);
		for (; ite != val.attachments.end(); ++ite)
		{
			hash ^= std::hash<int32_t>()(static_cast<uint32_t>(ite->get_format()) + 1) * 13;
		}
		return hash;
	}
};
