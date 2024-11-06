#pragma once
#include "macros.hpp"
#include "scene_component.hpp"
#include "scene\components\camera_component.gen.hpp"

namespace Engine
{
class CameraComponent: public SceneComponent
{
    REFLECT_BODY();
};

}
