#pragma once
#include "gfx/ui/ui_window.hpp"

#include <memory>
#include <glm/vec2.hpp>

namespace Eng
{
class Scene;
}

namespace Eng::Gfx
{
class RenderPassInstance;
}

class Viewport : public Eng::UiWindow
{
public:
    Viewport(const std::string& name, const std::weak_ptr<Eng::Gfx::RenderPassInstance>& in_render_pass, const std::shared_ptr<Eng::Scene> in_scene);

protected:
    void draw(Eng::Gfx::ImGuiWrapper& ctx) override;

    std::weak_ptr<Eng::Gfx::RenderPassInstance> render_pass;
    glm::uvec2                                  draw_res = {0, 0};
    std::shared_ptr<Eng::Scene>                 scene;
};