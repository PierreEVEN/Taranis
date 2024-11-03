#pragma once
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "gfx/types.hpp"

namespace Engine
{
	class CommandBuffer;
}

namespace Engine
{
	class Device;
	class RenderPassInstanceBase;
	class RenderPassInterface;
	class Swapchain;
	class Window;
	using RenderPassName = std::string;

	class RenderPassInterface
	{
	public:
		virtual void init(const std::weak_ptr<Device>& device, const RenderPassInstanceBase& render_pass) = 0;
		virtual void render(const RenderPassInstanceBase& render_pass, const CommandBuffer& command_buffer) = 0;
	};

	class DefaultRenderPassInterface : public RenderPassInterface
	{
	public:
		void init(const std::weak_ptr<Device>&, const RenderPassInstanceBase&) override {}
		void render(const RenderPassInstanceBase&, const CommandBuffer&) override {}
	};

	class ClearValue
	{
	public:
		static ClearValue none() { return {{}, {}}; }
		static ClearValue color(glm::vec4 color) { return {color, {}}; }
		static ClearValue depth_stencil(glm::vec2 depth_stencil) { return {{}, depth_stencil}; }
		bool is_none() const { return color_val && depth_stencil_val; }
		bool is_color() const { return color_val.has_value(); }
		bool is_depth_stencil() const { return depth_stencil_val.has_value(); }
		glm::vec4 color() const { return *color_val; }
		glm::vec2 depth_stencil() const { return *depth_stencil_val; }

	private:
		ClearValue(const std::optional<glm::vec4>& col, std::optional<glm::vec2> depth) : color_val(col),
			depth_stencil_val(depth)
		{
		}

		std::optional<glm::vec4> color_val;
		std::optional<glm::vec2> depth_stencil_val;
	};

	class Attachment
	{
	public:
		ColorFormat get_format() const { return format; }
		ClearValue clear_value() const { return clear_value_val; }
		bool is_depth() const { return is_depth_format(format); }

		static Attachment depth(std::string name, ColorFormat format, ClearValue clear_value = ClearValue::none())
		{
			Attachment attachment(std::move(name), clear_value);
			attachment.format = format;
			return attachment;
		}

		static Attachment color(std::string name, ColorFormat format, ClearValue clear_value = ClearValue::none())
		{
			Attachment attachment(std::move(name), clear_value);
			attachment.format = format;
			return attachment;
		}

		bool operator==(const Attachment& other) const
		{
			assert(format != ColorFormat::UNDEFINED);
			return get_format() == other.get_format() && clear_value().is_none() == other.clear_value().is_none();
		}

	private:
		Attachment(std::string in_name, const ClearValue& in_clear_value) :
			name(std::move(in_name)), clear_value_val(in_clear_value)
		{
		}

		std::string name;
		ColorFormat format = ColorFormat::UNDEFINED;
		ClearValue clear_value_val;
	};

	class RenderPassInfos
	{
	public:
		bool operator==(const RenderPassInfos& other) const
		{
			return attachments == other.attachments && present_pass == other.present_pass;
		}

		bool present_pass = false;
		std::vector<Attachment> attachments;
	};

	class RendererStep : public std::enable_shared_from_this<RendererStep>
	{
		friend class PresentStep;

	public:
		template <typename T = DefaultRenderPassInterface>
		static std::shared_ptr<RendererStep> create(RenderPassName name, std::vector<Attachment> in_attachments)
		{
			return std::shared_ptr<RendererStep>(new RendererStep(std::move(name), std::move(in_attachments), std::make_shared<T>()));
		}

		std::shared_ptr<RendererStep> attach(std::shared_ptr<RendererStep> dependency)
		{
			dependencies.emplace(std::move(dependency));
			return shared_from_this();
		}

		const RenderPassInfos& get_infos() const { return infos; }

		const std::unordered_set<std::shared_ptr<RendererStep>>& get_dependencies() const { return dependencies; }

		const std::shared_ptr<RenderPassInterface>& get_interface() const { return interface; }

	private:
		std::string pass_name;
		RenderPassInfos infos;
		std::unordered_set<std::shared_ptr<RendererStep>> dependencies;
		std::shared_ptr<RenderPassInterface> interface;

		RendererStep(RenderPassName name, std::vector<Attachment> in_attachments, const std::shared_ptr<RenderPassInterface>& in_interface) : pass_name(std::move(name)), interface(in_interface)
		{
			infos.attachments = std::move(in_attachments);
		}
	};

	class PresentStep : public std::enable_shared_from_this<PresentStep>
	{
	public:
		template <typename T = DefaultRenderPassInterface, typename...Args>
		static std::shared_ptr<PresentStep> create(RenderPassName name, ClearValue clear_value = ClearValue::none(), Args&&...args)
		{
			return std::shared_ptr<PresentStep>(new PresentStep(std::move(name), std::move(clear_value), std::make_shared<T>(std::forward<Args>(args)...)));
		}

		std::shared_ptr<PresentStep> attach(std::shared_ptr<RendererStep> dependency)
		{
			dependencies.emplace(std::move(dependency));
			return shared_from_this();
		}

		std::shared_ptr<RendererStep> init_for_swapchain(const Swapchain& swapchain) const;

	private:
		PresentStep(RenderPassName name, ClearValue in_clear_value, const std::shared_ptr<RenderPassInterface>& in_interface) : clear_value(in_clear_value),
		                                                              pass_name(std::move(name)), interface(in_interface)
		{
		}

		ClearValue clear_value;
		RenderPassName pass_name;
		std::shared_ptr<RenderPassInterface> interface;
		std::unordered_set<std::shared_ptr<RendererStep>> dependencies;
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
