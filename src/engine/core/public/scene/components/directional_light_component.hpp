#pragma once
#include "light_component.hpp"
#include "scene\components\directional_light_component.gen.hpp"

namespace Eng
{
class DirectionalLightComponent : public LightComponent
{
    REFLECT_BODY()

    DirectionalLightComponent();
    
};

}
