#include "scene/components/camera_component.hpp"

#include "engine.hpp"
#include "scene/scene_view.hpp"

#include <imgui.h>
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

SceneView& CameraComponent::get_view()
{
    if (!scene_view)
    {
        scene_view = SceneView::create();
        scene_view->set_position(get_relative_position());
        scene_view->set_rotation(get_relative_rotation());
        scene_view->set_fov(fov);
        scene_view->set_z_far(z_far);
        scene_view->set_z_near(z_near);
    }
    return *scene_view;

}

void CameraComponent::set_position(glm::vec3 in_position)
{
    get_view().set_position(in_position);
    SceneComponent::set_position(in_position);
}

void CameraComponent::set_rotation(glm::quat in_rotation)
{
    get_view().set_rotation(in_rotation);
    SceneComponent::set_rotation(in_rotation);
}

void CameraComponent::build_outliner(Gfx::ImGuiWrapper& ctx)
{
    SceneComponent::build_outliner(ctx);

    if (ImGui::Checkbox("Orthographic", &orthographic))
    {
        scene_view->set_perspective(!orthographic);
        scene_view->set_fov(orthographic ? orthographic_scale : fov);
    }
    if (ImGui::DragFloat(orthographic ? "Orthographic scale" : "Fov", orthographic ? &orthographic_scale : &fov))
        scene_view->set_fov(orthographic ? orthographic_scale : fov);
    if (ImGui::DragFloat("z_far", &z_far))
        scene_view->set_z_far(z_far);
    if (ImGui::DragFloat("z_near", &z_near))
        scene_view->set_z_near(z_near);
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
    set_rotation(glm::quat(glm::vec3{0, pitch, yaw}));
}
} // namespace Eng