#pragma once
#include "macros.hpp"
#include "scene_component.hpp"
#include "scene\components\camera_component.gen.hpp"

namespace Eng::Gfx
{
class RenderPassInstanceBase;
}

namespace Eng
{
class CameraComponent : public SceneComponent
{
    REFLECT_BODY();

public:
    CameraComponent()
    {
    };

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

    void update_viewport_resolution(const glm::uvec2& resolution);

private:
    void recompute();

    bool                                       outdated = true;
    float                                      fov      = 90.0f;
    float                                      z_near   = 0.01f;
    glm::mat4                                  view;
    glm::mat4                                  perspective;
    glm::mat4                                  perspective_view;
    glm::uvec2                                 resolution;
};


class FpsCameraComponent : public CameraComponent
{
    REFLECT_BODY();

public:
    FpsCameraComponent()
    {
    };

    void set_pitch(float in_pitch);
    void set_yaw(float in_yaw);

    float get_pitch() const
    {
        return pitch;
    }

    float get_yaw() const
    {
        return yaw;
    }

private:
    void update_rotation();

    float pitch = 0, yaw = 0;
};


}