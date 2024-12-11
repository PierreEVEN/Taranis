#include "scene/components/light_component.hpp"

#include "gfx/renderer/definition/renderer.hpp"
#include "gfx/renderer/instance/render_pass_instance.hpp"
#include "scene/scene_view.hpp"

#include <imgui.h>

namespace Eng
{
class SceneShadowsInterface : public Gfx::IRenderPass
{
public:
    SceneShadowsInterface(const std::shared_ptr<SceneView>& in_scene_view, Scene& in_scene) : scene_view(in_scene_view), scene(&in_scene)
    {
    }

    void pre_draw(const Gfx::RenderPassInstance& rp) override
    {
        scene_view->pre_draw(rp);
    }

    void draw(const Gfx::RenderPassInstance& rp, Gfx::CommandBuffer& command_buffer, size_t thread_index) override
    {
        scene_view->draw(*scene, rp, command_buffer, thread_index, record_threads());
    }

    size_t record_threads() override
    {
        return std::thread::hardware_concurrency() * 3;
    }

    void pre_submit(const Gfx::RenderPassInstance&) override
    {
        scene_view->pre_submit();
    }

    std::shared_ptr<SceneView> scene_view;
    Scene*                     scene = nullptr;
};

LightComponent::LightComponent() = default;

void LightComponent::enable_shadow(ELightType in_light_type, bool in_enabled)
{
    light_type = in_light_type;

    if (in_enabled == shadows)
        return;

    shadows = in_enabled;

    if (in_enabled)
    {
        shadow_view = SceneView::create();
        shadow_view->set_perspective(false);
        shadow_view->set_z_far(z_far);
        shadow_view->set_z_near(z_near);
        shadow_view->set_orthographic_width(orthographic_width);

        auto          obj_ref = as_ref();
        Gfx::Renderer renderer;
        renderer["shadows"]
            .render_pass<SceneShadowsInterface>(shadow_view, get_scene())
            .flip_culling(true)
            .resize_callback(
                [obj_ref](const glm::uvec2&) -> glm::uvec2
                {
                    return {obj_ref.cast<LightComponent>()->shadow_resolution, obj_ref.cast<LightComponent>()->shadow_resolution};
                })
            [Gfx::Attachment::slot("depth").format(Gfx::ColorFormat::D24_UNORM_S8_UINT).clear_depth({0.0f, 0.0f})];

        shadow_update_pass = get_scene().add_custom_pass({"gbuffer_resolve"}, renderer);
    }
    else
    {
        get_scene().remove_custom_pass(shadow_update_pass);
    }
}

void LightComponent::set_position(glm::vec3 in_position)
{
    SceneComponent::set_position(in_position);
    if (shadow_view)
        shadow_view->set_position(in_position);
}

void LightComponent::set_rotation(glm::quat in_rotation)
{
    SceneComponent::set_rotation(in_rotation);
    if (shadow_view)
        shadow_view->set_rotation(in_rotation);
}

void LightComponent::build_outliner(Gfx::ImGuiWrapper& ctx)
{
    SceneComponent::build_outliner(ctx);

    if (shadow_view)
    {
        if (ImGui::SliderFloat("Width", &orthographic_width, 10, 50000))
            shadow_view->set_orthographic_width(orthographic_width);
        if (ImGui::SliderFloat("Z near", &z_near, -50000, 0))
            shadow_view->set_z_near(z_near);
        if (ImGui::SliderFloat("Z far", &z_far, 0, 50000))
            shadow_view->set_z_far(z_far);
    }
}
} // namespace Eng