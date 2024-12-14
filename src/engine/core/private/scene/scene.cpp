#include <ranges>

#include "scene/scene.hpp"

#include "engine.hpp"
#include "gfx/vulkan/buffer.hpp"
#include "object_allocator.hpp"
#include "profiler.hpp"
#include "gfx/renderer/instance/render_pass_instance_base.hpp"
#include "scene/components/scene_component.hpp"

namespace Eng
{
Scene::Scene()
{
    merge_queue_mtx = std::make_unique<std::mutex>();
    allocator       = std::make_unique<ContiguousObjectAllocator>();
}

void Scene::tick(double delta_second)
{
    PROFILER_SCOPE(SceneTick);
    {
        PROFILER_SCOPE(MergeScenes);
        std::lock_guard lk(*merge_queue_mtx);
        for (auto& scene : scenes_to_merge)
        {
            scene.for_each<SceneComponent>(
                [&](SceneComponent& object)
                {
                    object.scene = this;
                });
            assert(scene.allocator);
            allocator->merge_with(*scene.allocator);
            root_nodes.reserve(root_nodes.size() + scene.root_nodes.size());
            for (const auto& component : scene.root_nodes)
                root_nodes.push_back(component);
            scene.root_nodes.clear();
        }
        scenes_to_merge.clear();
    }

    std::vector<std::vector<TObjectPtr<SceneComponent>>::iterator> deleted_nodes;
    for (auto node = root_nodes.begin(); node != root_nodes.end(); ++node)
    {
        if (!*node)
            deleted_nodes.push_back(node);
    }
    for (auto& deleted_node : std::ranges::reverse_view(deleted_nodes))
        root_nodes.erase(deleted_node);

    for_each<SceneComponent>(
        [delta_second](SceneComponent& object)
        {
            object.tick(delta_second);
        });
}

void Scene::merge(Scene&& other_scene)
{
    std::lock_guard lk(*merge_queue_mtx);
    scenes_to_merge.push_back(std::move(other_scene));
}

void Scene::set_pass_list(const std::weak_ptr<Gfx::CustomPassList>& pass_list)
{
    custom_passes = pass_list;
}

std::shared_ptr<Gfx::RenderPassInstanceBase> Scene::add_custom_pass(const std::vector<std::string>& targets, const Gfx::Renderer& node) const
{
    if (!custom_passes.lock())
    {
        LOG_ERROR("Scene is not attached to a renderer");
        return nullptr;
    }
    return custom_passes.lock()->add_custom_pass(targets, node);
}

void Scene::remove_custom_pass(const std::shared_ptr<Gfx::RenderPassInstanceBase>& pass) const
{
    if (!custom_passes.lock())
    {
        LOG_ERROR("Scene is not attached to a renderer");
        return;
    }
    return custom_passes.lock()->remove_custom_pass(pass->get_definition().render_pass_ref);
}

} // namespace Eng