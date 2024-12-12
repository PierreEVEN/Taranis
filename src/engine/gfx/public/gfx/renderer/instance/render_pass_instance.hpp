#pragma once

#include "gfx/renderer/definition/renderer.hpp"
#include "jobsys/job_sys.hpp"
#include "render_pass_instance_base.hpp"
#include "gfx/renderer/definition/render_pass_id.hpp"

#include <memory>
#include <string>

namespace Eng::Gfx
{
class CustomPassList;
class ImageView;
}

namespace Eng::Gfx
{
class Fence;
class Semaphore;
class Framebuffer;
class Swapchain;
class ImGuiWrapper;
class VkRendererPass;

class RenderPassInstance : public RenderPassInstanceBase
{
public:
    RenderPassInstance(std::weak_ptr<Device> device, const Renderer& renderer, const RenderPassGenericId& rp_ref, bool b_is_present);


    std::weak_ptr<VkRendererPass> get_render_pass_resource() const
    {
        return render_pass_resource;
    }

    ImGuiWrapper* imgui() const
    {
        return imgui_context.get();
    }

protected:
    void fill_command_buffer(CommandBuffer& cmd, size_t group_index) const override;
    void render_internal(SwapchainImageId swapchain_image, DeviceImageId device_image) override;

private:
    std::weak_ptr<VkRendererPass> render_pass_resource;
    std::unique_ptr<ImGuiWrapper> imgui_context;
};

class TemporaryRenderPassInstance : public RenderPassInstance
{
public:
    static std::shared_ptr<TemporaryRenderPassInstance> create(const std::weak_ptr<Device>& device, const Renderer& renderer);

    bool is_enabled() const
    {
        return enabled;
    }

    void enable(bool in_enabled)
    {
        enabled = in_enabled;
        if (!first_render)
            first_render = true;
    }

    void render(SwapchainImageId swapchain_image, DeviceImageId device_image) override;

private:
    bool first_render = true;
    bool enabled      = true;
    TemporaryRenderPassInstance(const std::weak_ptr<Device>& device, const Renderer& renderer);
};


class CustomPassList
{
public:
    CustomPassList(const std::weak_ptr<Device>& in_device) : device(in_device)
    {
    }

    std::shared_ptr<TemporaryRenderPassInstance> add_custom_pass(const std::vector<RenderPassGenericId>& targets, const Renderer& renderer)
    {
        const auto new_rp = TemporaryRenderPassInstance::create(device, renderer);
        for (const auto& target : targets)
            temporary_dependencies.emplace(target, std::vector<std::shared_ptr<TemporaryRenderPassInstance>>{}).first->second.emplace_back(new_rp);
        return new_rp;
    }

    void remove_custom_pass(const std::shared_ptr<TemporaryRenderPassInstance>& pass)
    {
        for (auto& dep : temporary_dependencies | std::views::values)
        {
            for (int64_t i = dep.size() - 1; i >= 0; --i)
                if (dep[i] == pass)
                    dep.erase(dep.begin() + i);
        }
    }

    std::vector<std::shared_ptr<TemporaryRenderPassInstance>> get_dependencies(const RenderPassGenericId& pass_name) const
    {
        if (auto found = temporary_dependencies.find(pass_name); found != temporary_dependencies.end())
            return found->second;
        return {};
    }

private:
    std::weak_ptr<Device>                                                                                        device;
    ankerl::unordered_dense::map<RenderPassGenericId, std::vector<std::shared_ptr<TemporaryRenderPassInstance>>> temporary_dependencies;
};

} // namespace Eng::Gfx