#pragma once
#include "scene_component.hpp"
#include "scene\components\mesh_component.gen.hpp"

namespace Eng
{
class CameraComponent;
}

namespace Eng
{
class MaterialInstanceAsset;
}

namespace Eng
{
class MeshAsset;
}

namespace Eng
{

class MeshComponent : public SceneComponent
{
    REFLECT_BODY();

public:
    MeshComponent(const TObjectRef<MeshAsset>& in_mesh = {})
        : mesh(in_mesh)
    {
    };

    void draw(Gfx::CommandBuffer& command_buffer);

    TObjectRef<MeshAsset>       mesh;
};

}