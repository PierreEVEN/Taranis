#pragma once
#include "macros.hpp"
#include "scene_component.hpp"
#include "scene\components\camera_component.gen.hpp"

namespace Engine::Gfx
{
class RenderPassInstanceBase;
}

namespace Engine
{
class CameraComponent: public SceneComponent
{
    REFLECT_BODY();

public:
    CameraComponent(const std::weak_ptr<Gfx::RenderPassInstanceBase>& target_render_pass) : render_pass(target_render_pass)
    {};


    const glm::mat4& view_matrix()
    {
        if (outdated)
            recompute();
        return view;
    }
    const glm::mat4& perspective_matrix()
    {
        if (outdated)
            recompute();
        return perspective;
    }
    const glm::mat4& perspective_view_matrix()
    {
        if (outdated)
            recompute();
        return perspective_view;
    }

private:

    void recompute();

    bool outdated = true;
    glm::mat4 view;
    glm::mat4 perspective;
    glm::mat4 perspective_view;
    std::weak_ptr<Gfx::RenderPassInstanceBase> render_pass;
};

}
