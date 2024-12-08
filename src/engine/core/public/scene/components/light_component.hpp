#pragma once
#include "macros.hpp"
#include "scene_component.hpp"

#include <glm/vec3.hpp>

#include "scene\components\light_component.gen.hpp"


namespace Eng
{
class SceneView;


enum class ELightType
{
    Movable,
    Stationary,
    Static,
};

class LightComponent : public SceneComponent
{
    REFLECT_BODY()

    LightComponent();

    void enable_shadow(ELightType light_type = ELightType::Stationary, bool enabled = true);

    const std::shared_ptr<Gfx::TemporaryRenderPassInstance>& get_shadow_pass() const
    {
        return shadow_update_pass;
    }

protected:
    void set_position(glm::vec3 in_position) override;
    void set_rotation(glm::quat in_rotation) override;

    virtual void void_create_shadow_resources()
    {

    }

    glm::vec3                                         color             = glm::vec3(1, 1, 1);
    uint32_t                                          shadow_resolution = 2048;
    bool                                              shadows           = false;
    ELightType                                        light_type        = ELightType::Stationary;
    std::shared_ptr<Gfx::TemporaryRenderPassInstance> shadow_update_pass;
    std::shared_ptr<SceneView>                        shadow_view;
    uint32_t                                          shadow_map_id = 0;


};
}