#include "scene/components/light_component.hpp"

namespace Eng
{

LightComponent::LightComponent()
{
}

void LightComponent::enable_shadow(ELightType in_light_type, bool in_enabled)
{
    light_type = in_light_type;
    shadows    = in_enabled;

    if (in_enabled)
    {



    }
}
} // namespace Eng