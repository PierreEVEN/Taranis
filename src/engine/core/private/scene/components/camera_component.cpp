#include "scene/components/camera_component.hpp"

#include "engine.hpp"
#include <numbers>

namespace Eng
{

void CameraComponent::update_viewport_resolution(const glm::uvec2& in_resolution)
{
    resolution = in_resolution;
}

void CameraComponent::recompute()
{
    //outdated = false;

    view = translate(mat4_cast(inverse(get_rotation())), -get_position());

    float aspect = static_cast<float>(resolution.x) / static_cast<float>(resolution.y);
    assert(std::abs(aspect - std::numeric_limits<float>::epsilon()) > static_cast<float>(0));
    float     h = 1.f / std::tan(glm::radians(fov) * 0.5f);
    float     w = h / aspect;
    perspective      = {
        {0, 0, 0, 1},
        {-w, 0, 0, 0},
        {0, h, 0, 0},
        {0, 0, z_near, 0}};

    perspective_view = perspective * view;
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