#pragma once
#include "gfx/ui/ui_window.hpp"

#include <glm/vec2.hpp>
#include <memory>

namespace Eng
{
class Scene;
}

namespace Eng::Gfx
{
class RenderPassInstanceBase;
}

class Viewport : public Eng::UiWindow
{
public:
    Viewport(const std::string& name, const std::weak_ptr<Eng::Gfx::RenderPassInstanceBase>& in_render_pass, const std::shared_ptr<Eng::Scene> in_scene);

protected:
    void draw(Eng::Gfx::ImGuiWrapper& ctx) override;

    std::weak_ptr<Eng::Gfx::RenderPassInstanceBase> render_pass;
    glm::uvec2                                      draw_res = {0, 0};
    std::shared_ptr<Eng::Scene>                     scene;
};