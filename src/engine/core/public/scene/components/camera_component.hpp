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
class SceneView;


class CameraComponent : public SceneComponent
{
    REFLECT_BODY();

public:
    CameraComponent();

    void activate();

    void tick(double) override;

    SceneView& get_view();

private:
public:
    void set_position(glm::vec3 in_position) override;
    void set_rotation(glm::quat in_rotation) override;

    void build_outliner(Gfx::ImGuiWrapper& ctx) override;

private:
    bool  orthographic       = false;
    float fov                = 90.f;
    float orthographic_scale = 1000.f;
    float z_near             = 0.1f;
    float z_far              = 10000.f;

    std::shared_ptr<SceneView> scene_view;
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

} // namespace Eng