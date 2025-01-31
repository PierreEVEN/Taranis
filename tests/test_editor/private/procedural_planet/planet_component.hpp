#pragma once
#include "scene/components/primitive_component.hpp"
#include "procedural_planet/planet_component.gen.hpp"

class FastNoise;

namespace Eng
{
namespace Gfx
{
class Mesh;
}

class MaterialInstanceAsset;
class SceneView;
}

class PlanetComponent : public Eng::PrimitiveComponent
{
public:
            
    REFLECT_BODY();

    PlanetComponent();

    virtual void draw(Eng::Gfx::CommandBuffer& command_buffer, const Eng::SceneView& view) override;

    std::shared_ptr<Eng::Gfx::Mesh>        mesh;
    TObjectRef<Eng::MaterialInstanceAsset> material;

private:
    std::shared_ptr<FastNoise> fast_noise;
};
