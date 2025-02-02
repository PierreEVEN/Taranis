#include "procedural_planet/planet_component.hpp"

#include "engine.hpp"
#include "assets/asset_registry.hpp"
#include "assets/material_asset.hpp"
#include "assets/material_instance_asset.hpp"
#include "gfx/mesh.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "scene/scene_view.hpp"
#include "scene/components/camera_component.hpp"

#include "third_party/fastnoise/FastNoise.h"

#include <numbers>

struct PlanetSectionVertex
{
    glm::vec3 pos;
    glm::vec2 uv;
    glm::vec3 norm;
};

static double make_noise(const glm::dvec3& source, const FastNoise& noise)
{
    return noise.GetSimplex(source.x * 1000.0, source.y * 1000.0, source.z * 1000.0) * 0.4
           + noise.GetSimplex(source.x * 200.0, source.y * 200.0, source.z * 200.0) * 0.7
           + (pow(noise.GetSimplex(source.x * 100.0, source.y * 100.0, source.z * 100.0), 12)) * 18.2;
}

uint32_t global_sec = 0;

PlanetSection::PlanetSection(PlanetComponent& in_root, uint32_t in_level, const glm::dmat3& in_transform, const glm::dvec2& in_offset) : transform(in_transform), offset(in_offset), root(in_root), level(in_level)
{
    constexpr size_t res      = 20;
    size_t           real_res = res + 2;
    constexpr double radius   = 100;

    const glm::dmat3 rot = glm::dmat3(in_transform);

    // Generate index buffer
    std::vector<uint32_t> indices;
    indices.reserve((res - 1) * (res - 1) * 6);
    for (size_t x = 1; x < res; ++x)
    {
        for (size_t y = 1; y < res; ++y)
        {

            indices.emplace_back(x + y * real_res);
            indices.emplace_back(x + 1 + (y + 1) * real_res);
            indices.emplace_back(x + 1 + y * real_res);

            indices.emplace_back(x + y * real_res);
            indices.emplace_back(x + (y + 1) * real_res);
            indices.emplace_back(x + 1 + (y + 1) * real_res);
        }
    }

    double scale = 2.0 / (1 << in_level);

    // Generate vertices
    std::vector<PlanetSectionVertex> vertices;
    vertices.reserve(real_res * real_res);
    for (size_t x = 0; x < real_res; ++x)
    {
        for (size_t y = 0; y < real_res; ++y)
        {
            glm::dvec3 pos = sphere_mapping(rot * glm::dvec3{
                                                (x - 1.0) * scale / static_cast<double>(res - 1) - 1.0 + in_offset.x,
                                                (y - 1.0) * scale / static_cast<double>(res - 1) - 1.0 + in_offset.y,
                                                1});

            const double z = make_noise(pos, *root.fast_noise);
            pos            = pos * (radius + z);

            vertices.emplace_back(PlanetSectionVertex{pos, {x, y}, {0, 0, 1}});
        }
    }

    // Compute normals
    for (size_t x = 1; x < res + 1; ++x)
    {
        for (size_t y = 1; y < res + 1; ++y)
        {
            const glm::dvec3& p = vertices[x + y * real_res].pos;

            glm::dvec3 pa = glm::dvec3(vertices[x + 1 + y * real_res].pos) - p;
            glm::dvec3 pb = glm::dvec3(vertices[x + (y + 1) * real_res].pos) - p;
            glm::dvec3 pc = glm::dvec3(vertices[x - 1 + y * real_res].pos) - p;
            glm::dvec3 pd = glm::dvec3(vertices[x + (y - 1) * real_res].pos) - p;

            glm::dvec3 n1 = cross(normalize(pb), normalize(pa));
            glm::dvec3 n2 = cross(normalize(pc), normalize(pb));
            glm::dvec3 n3 = cross(normalize(pd), normalize(pc));
            glm::dvec3 n4 = cross(normalize(pa), normalize(pd));

            vertices[x + y * real_res].norm = normalize((n1 + n2 + n3 + n4) / 4.0);
        }
    }

    auto device = Eng::Engine::get().get_device().lock();

    Eng::Gfx::BufferData index_buffer(indices);
    mesh = Eng::Gfx::Mesh::create("PlanetSection", device, Eng::Gfx::EBufferType::IMMUTABLE, Eng::Gfx::BufferData(vertices), &index_buffer);

    my_index = ++global_sec;
}

void PlanetSection::draw(Eng::Gfx::CommandBuffer& command_buffer, const Eng::SceneView& view)
{
    double scale         = 1.0 / (1 << level);
    auto   center = glm::dvec2{-1, -1} + offset + glm::dvec2(scale, scale);
    auto   sphere_center = sphere_mapping(transform * glm::dvec3{center, 1});
    const double z            = make_noise(sphere_center, *root.fast_noise);
    sphere_center *= (100.0 + z);
    double sphere_scale  = scale * 100.0;

    uint32_t desired_level = 0;

    root.get_scene().for_each<Eng::CameraComponent>(
        [&](const Eng::CameraComponent& camera)
        {
            auto d = distance(static_cast<glm::dvec3>(camera.get_relative_position()), sphere_center);
            desired_level = d < sphere_scale * 5 ? level + 1 : level;
        });

    if (desired_level == level)
        children.clear();
    if (desired_level > level && children.empty())
        subdivide();

    if (!children.empty())
        for (auto& child : children)
            child->draw(command_buffer, view);
    else
    {
        command_buffer.push_constant(Eng::Gfx::EShaderStage::Vertex, *root.base_material->get_permutation(root.base_material->get_default_permutation()).lock()->get_resource(command_buffer.render_pass()),
                                     Eng::Gfx::BufferData(level * 3));
        command_buffer.draw_mesh(*mesh);
    }
}

void PlanetSection::subdivide()
{
    if (level > 8)
        return;
    double scale = 1.0 / (1 << level);

    children.emplace_back(std::make_shared<PlanetSection>(root, level + 1, transform, offset));
    children.emplace_back(std::make_shared<PlanetSection>(root, level + 1, transform, offset + glm::dvec2{scale, scale}));
    children.emplace_back(std::make_shared<PlanetSection>(root, level + 1, transform, offset + glm::dvec2{0, scale}));
    children.emplace_back(std::make_shared<PlanetSection>(root, level + 1, transform, offset + glm::dvec2{scale, 0}));
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
    base_material_instance = Eng::Engine::get().asset_registry().create<Eng::MaterialInstanceAsset>("PlanetMaterialInst", base_material);

    roots.emplace_back(std::make_shared<PlanetSection>(*this, 0, mat3_cast(glm::quat(glm::vec3(0.f, 0.f, 0.f))), glm::dvec2{}));
    roots.emplace_back(std::make_shared<PlanetSection>(*this, 0, mat3_cast(glm::quat(glm::vec3(std::numbers::pi, 0.f, 0.f))), glm::dvec2{}));
    roots.emplace_back(std::make_shared<PlanetSection>(*this, 0, mat3_cast(glm::quat(glm::vec3(0.f, std::numbers::pi * -0.5f, 0.f))), glm::dvec2{}));
    roots.emplace_back(std::make_shared<PlanetSection>(*this, 0, mat3_cast(glm::quat(glm::vec3(0.f, std::numbers::pi * 0.5f, 0.f))), glm::dvec2{}));
    roots.emplace_back(std::make_shared<PlanetSection>(*this, 0, mat3_cast(glm::quat(glm::vec3(std::numbers::pi * -0.5f, 0.f, 0.f))), glm::dvec2{}));
    roots.emplace_back(std::make_shared<PlanetSection>(*this, 0, mat3_cast(glm::quat(glm::vec3(std::numbers::pi * 0.5f, 0.f, 0.f))), glm::dvec2{}));

}

void PlanetComponent::draw(Eng::Gfx::CommandBuffer& command_buffer, const Eng::SceneView& view)
{
    auto mat = base_material->get_permutation(base_material->get_default_permutation()).lock()->get_resource(command_buffer.render_pass());
    command_buffer.bind_pipeline(mat);
    base_material_instance->set_scene_data(command_buffer.render_pass(), view.get_view_buffer());
    command_buffer.bind_descriptors(*base_material_instance->get_descriptor_resource(command_buffer.render_pass()), *mat);

    for (auto& root : roots)
        root->draw(command_buffer, view);
}