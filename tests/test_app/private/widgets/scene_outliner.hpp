#pragma once
#include "object_ptr.hpp"
#include "gfx/ui/ui_window.hpp"

#include <memory>


namespace Eng
{
class SceneComponent;
}

namespace Eng
{
class Scene;
}

class SceneOutliner : public Eng::UiWindow
{
public:
    SceneOutliner(const std::string& name, const std::shared_ptr<Eng::Scene>& in_scene)
        : UiWindow(name)
    {
        scene = in_scene;
    }

protected:
    void draw(Eng::Gfx::ImGuiWrapper& ctx) override;

private:
    void                            display_node(const TObjectPtr<Eng::SceneComponent>& component);
    TObjectRef<Eng::SceneComponent> selected_component;
    size_t                          node_index = 0;
    std::shared_ptr<Eng::Scene>     scene;
};