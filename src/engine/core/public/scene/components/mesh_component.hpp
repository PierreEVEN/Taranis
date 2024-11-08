#pragma once
#include "scene_component.hpp"
#include "scene\components\mesh_component.gen.hpp"

namespace Engine
{
class MaterialInstanceAsset;
}

namespace Engine
{
class MeshAsset;
}

namespace Engine
{

class MeshComponent : public SceneComponent
{
    REFLECT_BODY();

public:
    MeshComponent(const TObjectRef<MeshAsset>& in_mesh = {}, const TObjectRef<MaterialInstanceAsset>& in_material = {}) : material(in_material), mesh(in_mesh)
    {
    };

    void draw(const Gfx::CommandBuffer& command_buffer) const;

    TObjectRef<MaterialInstanceAsset> material;
    TObjectRef<MeshAsset>             mesh;
};

}