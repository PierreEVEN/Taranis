#pragma once
#include "attachment.hpp"
#include "interface.hpp"

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace Engine::Gfx
{
class Attachment;

using RenderPassPtr = std::shared_ptr<class RenderPass>;
using RenderPassRef = std::weak_ptr<class RenderPass>;
class RenderPass : public std::enable_shared_from_this<RenderPass>
{
    friend class Renderer;

  public:
    class Definition
    {
      public:
        bool operator==(const Definition& other) const
        {
            return attachments == other.attachments && present_pass == other.present_pass;
        }

        bool                    present_pass = false;
        std::vector<Attachment> attachments;
        std::string             name;
    };

    template <typename T = DefaultRenderPassInterface, typename... Args> static RenderPassPtr create(std::string name, std::vector<Attachment> in_attachments, Args&&... args)
    {
        return RenderPassPtr(new RenderPass(std::move(name), std::move(in_attachments), std::make_shared<T>(std::forward<Args>(args)...)));
    }

    RenderPassPtr attach(RenderPassPtr dependency)
    {
        dependencies.emplace(std::move(dependency));
        return shared_from_this();
    }

    const Definition& get_infos() const
    {
        return infos;
    }

    const std::unordered_set<RenderPassPtr>& get_dependencies() const
    {
        return dependencies;
    }

    const std::shared_ptr<RenderPassInterface>& get_interface() const
    {
        return interface;
    }

    const std::string& get_name() const
    {
        return pass_name;
    }

    RenderPass(RenderPass&)  = delete;
    RenderPass(RenderPass&&) = delete;

  private:
    std::string                          pass_name;
    Definition                           infos;
    std::unordered_set<RenderPassPtr>    dependencies;
    std::shared_ptr<RenderPassInterface> interface;

    RenderPass(std::string name, std::vector<Attachment> in_attachments, const std::shared_ptr<RenderPassInterface>& in_interface) : pass_name(std::move(name)), interface(in_interface)
    {
        infos.attachments = std::move(in_attachments);
        infos.name        = pass_name;
    }
};
} // namespace Engine::Gfx

template <> struct std::hash<Engine::Gfx::RenderPass::Definition>
{
    size_t operator()(const Engine::Gfx::RenderPass::Definition& val) const noexcept
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