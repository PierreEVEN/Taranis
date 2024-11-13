#include <ranges>

#include "scene/scene.hpp"

#include "object_allocator.hpp"
#include "profiler.hpp"
#include "scene/components/mesh_component.hpp"
#include "scene/components/scene_component.hpp"

namespace Eng
{
Scene::Scene()
{
    merge_queue_mtx = std::make_unique<std::mutex>();
    allocator = std::make_unique<ContiguousObjectAllocator>();
}

void Scene::tick(double delta_second)
{
    {
        PROFILER_SCOPE(MergeScenes);
        std::lock_guard lk(*merge_queue_mtx);
        for (auto& scene : scenes_to_merge)
        {
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

    for_each<SceneComponent>([delta_second](SceneComponent& object)
    {
        object.tick(delta_second);
    });
}

void Scene::draw(Gfx::CommandBuffer& cmd_buffer)
{
    for_each<MeshComponent>([&cmd_buffer](MeshComponent& object)
        {
            object.draw(cmd_buffer);
        });
}
}