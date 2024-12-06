#include "scene/components/scene_component.hpp"

#include <imgui.h>

#include <ranges>

namespace Eng
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

void SceneComponent::build_outliner(Gfx::ImGuiWrapper& ctx)
{
    glm::vec3 out_pos = get_relative_position();
    ImGui::DragFloat3("position", &out_pos.x, 10);
    if (out_pos != get_relative_position())
        set_position(out_pos);

    glm::vec3 base_rot = eulerAngles(get_relative_rotation());
    glm::vec3 out_rot  = base_rot;
    ImGui::DragFloat3("rotation", &out_rot.x, 0.02f);
    if (out_rot != base_rot)
        set_rotation(out_rot);
};
} // namespace Eng