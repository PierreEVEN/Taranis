#include <ranges>

#include "scene/scene.hpp"

#include "object_allocator.hpp"
#include "scene/components/mesh_component.hpp"
#include "scene/components/scene_component.hpp"

namespace Engine
{
Scene::Scene()
{
    allocator = std::make_unique<ContiguousObjectAllocator>();
}

void Scene::tick(double delta_second)
{
    std::vector<std::vector<TObjectPtr<SceneComponent>>::iterator> deleted_nodes;
    for (auto node = root_nodes.begin(); node != root_nodes.end(); ++node)
    {
        if (!*node)
            deleted_nodes.push_back(node);
    }
    for (auto& deleted_node : std::ranges::reverse_view(deleted_nodes))
        root_nodes.erase(deleted_node);

    for (auto iterator = iterate<SceneComponent>(); iterator; ++iterator)
        iterator->tick(delta_second);
}

void Scene::draw(const Gfx::CommandBuffer& cmd_buffer)
{

    for (auto iterator = iterate<MeshComponent>(); iterator; ++iterator)
        iterator->draw(cmd_buffer);


}
}