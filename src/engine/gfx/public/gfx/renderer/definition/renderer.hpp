#pragma once

#include "attachment.hpp"
#include "interface.hpp"

#include <memory>
#include <unordered_set>

namespace Engine::Gfx
{
class Swapchain;
class Renderer;
class RenderPass;

using RendererPtr = std::shared_ptr<Renderer>;
using RendererRef = std::weak_ptr<Renderer>;

class Renderer : public std::enable_shared_from_this<Renderer>
{
  public:
    struct CreateInfos
    {
        ClearValue clear_color = ClearValue::none();
        bool       b_enable_ui = false;
    };

    template <typename T = DefaultRenderPassInterface, typename... Args> static std::shared_ptr<Renderer> create(std::string name, const CreateInfos& create_infos, Args&&... args)
    {
        return RendererPtr(new Renderer(std::move(name), create_infos, std::make_shared<T>(std::forward<Args>(args)...)));
    }

    RendererPtr attach(const std::shared_ptr<RenderPass>& dependency)
    {
        dependencies.emplace(dependency);
        return shared_from_this();
    }

    std::shared_ptr<RenderPass> init_for_swapchain(const Swapchain& swapchain) const;

    Renderer(Renderer&)  = delete;
    Renderer(Renderer&&) = delete;

  private:
    Renderer(std::string name, const CreateInfos& in_create_infos, const std::shared_ptr<RenderPassInterface>& in_interface) : create_infos(in_create_infos), pass_name(std::move(name)), interface(in_interface)
    {
    }

    CreateInfos                                     create_infos;
    std::string                                     pass_name;
    std::shared_ptr<RenderPassInterface>            interface;
    std::unordered_set<std::shared_ptr<RenderPass>> dependencies;
};
} // namespace Engine::Gfx