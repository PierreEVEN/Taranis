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

  private:
    std::shared_ptr<SceneView> scene_view;
};

class FpsCameraComponent : public CameraComponent
{
    REFLECT_BODY();

  public:
    FpsCameraComponent(){};

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