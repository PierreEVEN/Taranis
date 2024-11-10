#include "scene_outliner.hpp"

#include "scene/scene.hpp"
#include "scene/components/scene_component.hpp"

#include <imgui.h>

void SceneOutliner::draw(Eng::Gfx::ImGuiWrapper&)
{
    node_index = 0;
    for (const auto& node : scene->get_nodes())
    {
        if (!node)
            continue;

        display_node(node);
    }


}

void SceneOutliner::display_node(const TObjectPtr<Eng::SceneComponent>& component)
{
    int flags = ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (component->get_nodes().empty())
        flags |= ImGuiTreeNodeFlags_Leaf;
    if (component == selected_component)
        flags |= ImGuiTreeNodeFlags_Selected;
    const bool b_expand = ImGui::TreeNodeEx((component->get_name() + std::string("##") + std::to_string(node_index++)).c_str(), flags);

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        selected_component = component;
    }

    if (b_expand)
    {
        for (const auto& child : component->get_nodes())
        {
            if (!child)
                continue;

            display_node(child);
        }
        ImGui::TreePop();
    }
}