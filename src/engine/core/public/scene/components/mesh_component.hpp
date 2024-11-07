#pragma once
#include "scene_component.hpp"
#include "scene\components\mesh_component.gen.hpp"

namespace Engine
{

class MeshComponent : public SceneComponent
{
    REFLECT_BODY();

public:
    MeshComponent(){};
};

}
