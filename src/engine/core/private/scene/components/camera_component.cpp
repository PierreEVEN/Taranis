#include "scene/components/camera_component.hpp"

#include "gfx/renderer/instance/render_pass_instance.hpp"

#include "engine.hpp"
#include <numbers>

namespace Eng
{

void CameraComponent::recompute()
{
    //outdated = false;

    auto res = render_pass.lock()->resolution();

    float pi = std::numbers::pi_v<float>;

    view = translate(mat4_cast(glm::quat(glm::vec3{0, 0, -0}) * get_rotation()), -get_position());

    glm::vec3 forward = get_rotation() * glm::vec3(1, 0, 0);
    glm::vec3 right   = get_rotation() * glm::vec3(0, 1, 0);
    glm::vec3 up      = get_rotation() * glm::vec3(0, 0, 1);

    perspective      = glm::perspective(90.f, static_cast<float>(res.x) / static_cast<float>(res.y), 0.1f, 10000.f);
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

    set_rotation(glm::quat(glm::vec3{0, pitch, 0}) * glm::quat(glm::vec3{0, 0, yaw}));

}
} // namespace Eng