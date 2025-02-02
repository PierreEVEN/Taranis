#pragma once
#include "scene/components/primitive_component.hpp"
#include "procedural_planet/planet_component.gen.hpp"

struct PlanetSectionVertex;
class FastNoise;

namespace Eng
{
class MaterialAsset;

namespace Gfx
{
class Mesh;
}

class MaterialInstanceAsset;
class SceneView;
}

class PlanetSection
{
public:
    PlanetSection(PlanetComponent& in_root, uint32_t in_level, const glm::dmat3& in_transform, const glm::dvec2& in_offset);

    void draw(Eng::Gfx::CommandBuffer& command_buffer, const Eng::SceneView& view);

    static glm::dvec3 sphere_mapping(const glm::dvec3& source)
    {
        auto x2 = source.x * source.x;
        auto y2 = source.y * source.y;
        auto z2 = source.z * source.z;

        return {source.x * std::sqrt(1 - (y2 + z2) / 2 + y2 * z2 / 3),
                source.y * std::sqrt(1 - (x2 + z2) / 2 + x2 * z2 / 3),
                source.z * std::sqrt(1 - (y2 + x2) / 2 + y2 * x2 / 3)};
    }

    void subdivide();

private:
    std::vector<std::shared_ptr<PlanetSection>> children;

    glm::dmat3       transform;
    glm::dvec2       offset;
    PlanetComponent& root;
    uint32_t         level = 0;
    uint32_t           my_index = 0;

    std::shared_ptr<Eng::Gfx::Mesh> mesh;
};

class PlanetComponent : public Eng::PrimitiveComponent
{
public:
    REFLECT_BODY();

    PlanetComponent();

    void draw(Eng::Gfx::CommandBuffer& command_buffer, const Eng::SceneView& view) override;

    TObjectRef<Eng::MaterialAsset>         base_material;
    TObjectRef<Eng::MaterialInstanceAsset> base_material_instance;

    std::shared_ptr<FastNoise> fast_noise;

private:
    std::vector<std::shared_ptr<PlanetSection>> roots;
};