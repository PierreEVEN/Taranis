#include "procedural_planet/planet_component.hpp"

#include "engine.hpp"
#include "assets/asset_registry.hpp"
#include "assets/material_asset.hpp"
#include "assets/material_instance_asset.hpp"
#include "gfx/mesh.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "scene/scene_view.hpp"

#include "third_party/fastnoise/FastNoise.h"

#include <numbers>

struct PlanetSectionVertex
{
    glm::vec3 pos;
    glm::vec2 uv;
    glm::vec3 norm;
};

struct PlanetPoint
{
    float altitude;

};

static float make_noise(const glm::vec3& source, const FastNoise& noise)
{
    return noise.GetSimplex(source.x * 1000.f, source.y * 1000.f, source.z * 1000.f) * 0.4f
    + noise.GetSimplex(source.x * 200.f, source.y * 200.f, source.z * 200.f) * 0.7f
    + noise.GetSimplex(source.x * 100.f, source.y * 100.f, source.z * 100.f) * 4.2f;
}

PlanetSection::PlanetSection(PlanetComponent& in_root, int in_level, const glm::mat4& in_transform) : transform(in_transform), root(in_root), level(in_level)
{
    uint32_t                         res = 200;
    std::vector<PlanetSectionVertex> vertices;
    std::vector<uint32_t>            indices;
    generate_square_section(vertices, indices, res);
    auto rot = glm::mat3(in_transform);

    std::vector<PlanetPoint> points;
    points.reserve(res * res);

    for (auto& vertex : vertices)
    {
        vertex.pos = rot * vertex.pos;

        const float z = make_noise(vertex.pos, *root.fast_noise);
        points.emplace_back(z);
        vertex.pos    = sphere_mapping(vertex.pos) * (100 + z);
    }
    generate_normals(vertices, res);

    auto device = Eng::Engine::get().get_device().lock();

    terrain_data = Eng::Gfx::Buffer::create("SectionData", device, {.usage = Eng::Gfx::EBufferUsage::GPU_MEMORY, .type = Eng::Gfx::EBufferType::IMMUTABLE}, Eng::Gfx::BufferData(points));
    material     = Eng::Engine::get().asset_registry().create<Eng::MaterialInstanceAsset>("PlanetMaterialInst", root.base_material);
    material->set_buffer("data", terrain_data);

}

void PlanetSection::generate_square_section(std::vector<PlanetSectionVertex>& vertices, std::vector<uint32_t>& indices, uint32_t res)
{
    // Generate vertices
    for (size_t x = 0; x < res; ++x)
    {
        for (size_t y = 0; y < res; ++y)
        {
            float p_x = static_cast<float>(x) / static_cast<float>(res - 1) * 2.f - 1.f;
            float p_y = static_cast<float>(y) / static_cast<float>(res - 1) * 2.f - 1.f;
            float p_z = 1;
            vertices.emplace_back(PlanetSectionVertex{glm::vec3{p_x, p_y, p_z}, {x, y}, {0, 0, 1}});
        }
    }
    // Generate index buffer
    for (size_t x = 0; x < res - 1; ++x)
    {
        for (size_t y = 0; y < res - 1; ++y)
        {
            indices.emplace_back(x + y * res);
            indices.emplace_back(x + 1 + (y + 1) * res);
            indices.emplace_back(x + 1 + y * res);

            indices.emplace_back(x + y * res);
            indices.emplace_back(x + (y + 1) * res);
            indices.emplace_back(x + 1 + (y + 1) * res);
        }
    }
}

void PlanetSection::draw(Eng::Gfx::CommandBuffer& command_buffer, const Eng::SceneView& view)
{
    if (!children.empty())
        for (auto& child : children)
            child.draw(command_buffer, view);
    else
    {
        material->set_scene_data(command_buffer.render_pass(), view.get_view_buffer());
        command_buffer.push_constant(Eng::Gfx::EShaderStage::Vertex, *root.base_material, Eng::Gfx::BufferData(glm::identity<glm::mat4>()));
        command_buffer.bind_descriptors(*material->get_descriptor_resource(command_buffer.render_pass()), *root.base_material);
        command_buffer.draw_mesh(*root.base_mesh);
    }
}

void PlanetSection::generate_normals(std::vector<PlanetSectionVertex>& vertices, uint32_t res)
{
    // Compute normals
    for (size_t x = 1; x < res - 1; ++x)
    {
        for (size_t y = 1; y < res - 1; ++y)
        {
            const glm::vec3& p = vertices[x + y * res].pos;

            glm::vec3 pa = vertices[x + 1 + y * res].pos - p;
            glm::vec3 pb = vertices[x + (y + 1) * res].pos - p;
            glm::vec3 pc = vertices[x - 1 + y * res].pos - p;
            glm::vec3 pd = vertices[x + (y - 1) * res].pos - p;

            glm::vec3 n1 = cross(normalize(pb), normalize(pa));
            glm::vec3 n2 = cross(normalize(pd), normalize(pc));

            vertices[x + y * res].norm = normalize((n1 + n2) / 2.f);
        }
    }
}

glm::vec3 PlanetSection::sphere_mapping(const glm::vec3& source)
{
    return {source.x * std::sqrt(1 - source.y * source.y / 2 - source.z * source.z / 2 + source.y * source.y * source.z * source.z / 3),
            source.y * std::sqrt(1 - source.x * source.x / 2 - source.z * source.z / 2 + source.x * source.x * source.z * source.z / 3),
            source.z * std::sqrt(1 - source.y * source.y / 2 - source.x * source.x / 2 + source.y * source.y * source.x * source.x / 3)};
}

PlanetComponent::PlanetComponent()
{
    fast_noise = std::make_shared<FastNoise>();

    auto device   = Eng::Engine::get().get_device().lock();
    base_material = Eng::Engine::get().asset_registry().create<Eng::MaterialAsset>("PlanetMaterial");
    base_material->set_shader_code("planet_mat", std::vector{
                                  StageInputOutputDescription{0, 0, Eng::Gfx::ColorFormat::R32G32B32_SFLOAT},
                                  StageInputOutputDescription{1, 12, Eng::Gfx::ColorFormat::R32G32_SFLOAT},
                                  StageInputOutputDescription{2, 20, Eng::Gfx::ColorFormat::R32G32B32_SFLOAT},
                              });

    Eng::Gfx::BufferData index_buffer(indices);
    base_mesh = Eng::Gfx::Mesh::create("PlanetSection", device, Eng::Gfx::EBufferType::IMMUTABLE, Eng::Gfx::BufferData(vertices), &index_buffer);

    roots.emplace_back(std::make_shared<PlanetSection>(*this, 0, mat4_cast(glm::quat(glm::vec3(0.f, 0.f, 0.f)))));
    roots.emplace_back(std::make_shared<PlanetSection>(*this, 0, mat4_cast(glm::quat(glm::vec3(std::numbers::pi, 0.f, 0.f)))));
    roots.emplace_back(std::make_shared<PlanetSection>(*this, 0, mat4_cast(glm::quat(glm::vec3(0.f, std::numbers::pi * -0.5f, 0.f)))));
    roots.emplace_back(std::make_shared<PlanetSection>(*this, 0, mat4_cast(glm::quat(glm::vec3(0.f, std::numbers::pi * 0.5f, 0.f)))));
    roots.emplace_back(std::make_shared<PlanetSection>(*this, 0, mat4_cast(glm::quat(glm::vec3(std::numbers::pi * -0.5f, 0.f, 0.f)))));
    roots.emplace_back(std::make_shared<PlanetSection>(*this, 0, mat4_cast(glm::quat(glm::vec3(std::numbers::pi * 0.5f, 0.f, 0.f)))));

}

void PlanetComponent::draw(Eng::Gfx::CommandBuffer& command_buffer, const Eng::SceneView& view)
{
    auto mat = base_material->get_permutation(base_material->get_default_permutation()).lock()->get_resource(command_buffer.render_pass());
    command_buffer.bind_pipeline(mat);

    uint32_t                         res = 200;
    std::vector<PlanetSectionVertex> vertices;
    std::vector<uint32_t>            indices;
    PlanetSection::generate_square_section(vertices, indices, res);
    auto rot = glm::mat3(in_transform);

    std::vector<PlanetPoint> points;
    points.reserve(res * res);

    for (auto& root : roots)
        root->draw(command_buffer, view);
}