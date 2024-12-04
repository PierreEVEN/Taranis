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

    void pre_submit(const Gfx::RenderPassInstance&) override
    {
        scene_view->pre_submit();
    }

    std::shared_ptr<SceneView> scene_view;
    Scene*                     scene = nullptr;
};


LightComponent::LightComponent()
{
}

class ShadowIds
{
public:
    uint32_t acquire()
    {
        if (available.empty())
            return ++max;
        auto last = available.back();
        available.pop_back();
        return last;
    }

    void release(uint32_t& id)
    {
        available.emplace_back(id);
        id = 0;
    }

private:
    std::vector<uint32_t> available;
    uint32_t              max = 0;
};

static ShadowIds SHADOW_IDS;

void LightComponent::enable_shadow(ELightType in_light_type, bool in_enabled)
{
    light_type = in_light_type;

    if (in_enabled == shadows)
        return;

    if (in_enabled)
    {
        Gfx::Renderer renderer;
        shadow_map_id = SHADOW_IDS.acquire();
        renderer["shadows_#" + std::to_string(shadow_map_id)]
            .render_pass<SceneShadowsInterface>(get_scene())
            .resize_callback(
                [this](const glm::uvec2&)-> glm::uvec2
                {
                    return {shadow_resolution, shadow_resolution};
                })
            [Gfx::Attachment::slot("depth").format(Gfx::ColorFormat::D32_SFLOAT).clear_depth({0.0f, 0.0f})];

        shadow_update_pass = get_scene().add_custom_pass({"gbuffer_resolve"}, renderer);
    }
    else
    {
        get_scene().remove_custom_pass(shadow_update_pass);
        SHADOW_IDS.release(shadow_map_id);
    }
}
} // namespace Eng