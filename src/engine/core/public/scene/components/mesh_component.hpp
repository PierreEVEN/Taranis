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
    MeshComponent(const TObjectRef<CameraComponent>& in_temp_cam, const TObjectRef<MeshAsset>& in_mesh = {})
        : mesh(in_mesh), temp_cam(in_temp_cam)
    {
    };

    void draw(const Gfx::CommandBuffer& command_buffer);

    TObjectRef<MeshAsset>       mesh;
    TObjectRef<CameraComponent> temp_cam;
};

}