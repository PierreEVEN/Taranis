#include "scene/components/camera_component.hpp"

#include "gfx/renderer/instance/render_pass_instance.hpp"

#include "engine.hpp"
#include <numbers>

#include <glm/ext/matrix_clip_space.hpp>

namespace Eng
{

void CameraComponent::recompute()
{
    //outdated = false;

    view = translate(mat4_cast(inverse(get_rotation())), -get_position());

    auto  res    = render_pass.lock()->resolution();
    float aspect = static_cast<float>(res.x) / static_cast<float>(res.y);
    assert(std::abs(aspect - std::numeric_limits<float>::epsilon()) > static_cast<float>(0));
    float     h = 1.f / std::tan(glm::radians(fov) * 0.5f);
    float     w = h / aspect;
    perspective      = {
        {0, 0, 0, 1},
        {w, 0, 0, 0},
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