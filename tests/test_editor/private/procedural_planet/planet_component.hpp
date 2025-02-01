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
    PlanetSection(PlanetComponent& in_root, int in_level, const glm::mat4& in_transform);

    void draw(Eng::Gfx::CommandBuffer& command_buffer, const Eng::SceneView& view);

    static void      generate_square_section(std::vector<PlanetSectionVertex>& vertices, std::vector<uint32_t>& indices, uint32_t res);
    static void      generate_normals(std::vector<PlanetSectionVertex>& vertices, uint32_t res);
    static glm::vec3 sphere_mapping(const glm::vec3& source);

private:
    std::vector<PlanetSection> children;

    glm::mat4        transform;
    PlanetComponent& root;
    int              level = 0;

    std::shared_ptr<Eng::Gfx::Buffer>      terrain_data;
    TObjectRef<Eng::MaterialInstanceAsset> material;
};

class PlanetComponent : public Eng::PrimitiveComponent
{
public:
    REFLECT_BODY();

    PlanetComponent();

    void draw(Eng::Gfx::CommandBuffer& command_buffer, const Eng::SceneView& view) override;

    TObjectRef<Eng::MaterialAsset>  base_material;
    std::shared_ptr<Eng::Gfx::Mesh> base_mesh;

    std::shared_ptr<FastNoise> fast_noise;

private:
    std::vector<std::shared_ptr<PlanetSection>> roots;
};