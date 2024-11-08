
#include "scene/components/camera_component.hpp"

#include "gfx/renderer/instance/render_pass_instance.hpp"

#include "engine.hpp"
#include <numbers>

namespace Engine
{

void CameraComponent::recompute()
{
    //outdated = false;

    auto res = render_pass.lock()->resolution();

    view = glm::identity<glm::mat4>();
    view = translate(view, get_position());

    perspective      = glm::perspectiveRH(90.f, static_cast<float>(res.x) / static_cast<float>(res.y), 0.1f, 10000.f);
    perspective_view = transpose(perspective * view);
}
} // namespace Engine