#include "scene/components/camera_component.hpp"

#include "engine.hpp"
#include <numbers>

namespace Eng
{

CameraComponent::CameraComponent()
{
}

void CameraComponent::activate()
{
    get_scene().set_active_camera(get_scene().get_component_ref(this));
}

void CameraComponent::tick(double x)
{
    SceneComponent::tick(x);

    if (!get_scene().get_active_camera())
        activate();
}

void CameraComponent::recompute()
{
    // outdated = false;

    view = translate(mat4_cast(inverse(get_rotation())), -get_position());

    if (resolution.x == 0 || resolution.y == 0)
        return;
    float aspect = static_cast<float>(resolution.x) / static_cast<float>(resolution.y);
    assert(std::abs(aspect - std::numeric_limits<float>::epsilon()) > static_cast<float>(0));
    float h     = 1.f / std::tan(glm::radians(fov) * 0.5f);
    float w     = h / aspect;
    perspective = {{0, 0, 0, 1}, {-w, 0, 0, 0}, {0, h, 0, 0}, {0, 0, z_near, 0}};

    perspective_view = perspective * view;
}

void CameraComponent::set_position(glm::vec3 in_position)
{
    if (in_position == get_position())
        return;
    SceneComponent::set_position(in_position);
    outdated = true;
}

void CameraComponent::set_rotation(glm::quat in_rotation)
{
    if (in_rotation == get_rotation())
        return;
    SceneComponent::set_rotation(in_rotation);
    outdated = true;
}

void FpsCameraComponent::set_pitch(float in_pitch)
{
    pitch = std::clamp(in_pitch, -std::numbers::pi_v<float> / 2, std::numbers::pi_v<float> / 2);
    update_rotation();
}

void FpsCameraComponent::set_yaw(float in_yaw)
{
    yaw = in_yaw;
    update_rotation();
}

void FpsCameraComponent::update_rotation()
{
    set_rotation(glm::quat(glm::vec3{0, 0, 0}) * glm::quat(glm::vec3{0, pitch, yaw}));
}
} // namespace Eng