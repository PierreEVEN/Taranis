#include "scene/components/scene_component.hpp"

#include <ranges>

namespace Engine
{
void SceneComponent::internal_tick(double delta_second)
{
    std::vector<std::vector<TObjectPtr<SceneComponent>>::iterator> deleted_nodes;
    for (auto node = children.begin(); node != children.end(); ++node)
    {
        if (*node)
            (*node)->tick(delta_second);
        else
            deleted_nodes.push_back(node);
    }

    for (auto& deleted_node : std::ranges::reverse_view(deleted_nodes))
        children.erase(deleted_node);
    tick(delta_second);
}


}