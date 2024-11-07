#include <ranges>

#include "scene/scene.hpp"

#include "scene/components/scene_component.hpp"

namespace Engine
{

void Scene::tick(double delta_second)
{
    std::vector<std::vector<TObjectPtr<SceneComponent>>::iterator> deleted_nodes;
    for (auto node = root_nodes.begin(); node != root_nodes.end(); ++node)
    {
        if (*node)
            (*node)->internal_tick(delta_second);
        else
            deleted_nodes.push_back(node);
    }

    for (auto& deleted_node : std::ranges::reverse_view(deleted_nodes))
        root_nodes.erase(deleted_node);
}
}