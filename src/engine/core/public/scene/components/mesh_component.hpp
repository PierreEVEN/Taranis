#pragma once
#include "scene_component.hpp"

#include "scene\components\mesh_component.gen.hpp"

namespace Eng
{
class SceneView;
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
    MeshComponent(const TObjectRef<MeshAsset>& in_mesh = {}) : mesh(in_mesh){};

    void draw(Gfx::CommandBuffer& command_buffer, const SceneView& view);

    TObjectRef<MeshAsset> mesh;
};

} // namespace Eng