#include "scene/components/light_component.hpp"

#include "gfx/renderer/definition/renderer.hpp"
#include "scene/scene_view.hpp"

namespace Eng
{
class SceneShadowsInterface : public Gfx::IRenderPass
{
public:
    SceneShadowsInterface(Scene& in_scene) : scene_view(SceneView::create()), scene(&in_scene)
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

    void pre_submit(const Gfx::RenderPassInstance& rp) override
    {
        scene_view->pre_submit();
    }

    std::shared_ptr<SceneView> scene_view;
    Scene*                     scene = nullptr;
};


LightComponent::LightComponent()
{
}

void LightComponent::enable_shadow(ELightType in_light_type, bool in_enabled)
{
    light_type = in_light_type;
    shadows    = in_enabled;

    if (in_enabled)
    {
        Gfx::Renderer renderer;

        renderer["shadows"]
            .render_pass<SceneShadowsInterface>(get_scene())
            [Gfx::Attachment::slot("depth").format(Gfx::ColorFormat::D32_SFLOAT).clear_depth({0.0f, 0.0f})];

        shadow_update_pass = get_scene().add_custom_pass({"gbuffer_resolve"}, renderer);
    }
    else
    {
        get_scene().remove_custom_pass(shadow_update_pass);
    }
}
} // namespace Eng