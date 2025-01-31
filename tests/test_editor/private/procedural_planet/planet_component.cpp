#include "procedural_planet/planet_component.hpp"

#include "engine.hpp"
#include "assets/asset_registry.hpp"
#include "assets/material_asset.hpp"
#include "assets/material_instance_asset.hpp"
#include "gfx/mesh.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "scene/scene_view.hpp"

#include "third_party/fastnoise/FastNoise.h"

PlanetComponent::PlanetComponent()
{
    fast_noise = std::make_shared<FastNoise>();

    struct PlanetSectionVertex
    {
        glm::vec3 pos;
        glm::vec2 uv;
        glm::vec3 norm;
    };

    std::vector<PlanetSectionVertex> vertices;
    std::vector<uint32_t>            indices;

    size_t width = 1000;

    constexpr float mesh_scale = 50.f;

    // Generate vertices
    for (size_t x = 0; x < width; ++x)
    {
        for (size_t y = 0; y < width; ++y)
        {
            float p_x  = (static_cast<float>(x) / static_cast<float>(width) * 2.f - 1.f);
            float p_y  = (static_cast<float>(y) / static_cast<float>(width) * 2.f - 1.f);
            float p_z  = 1;
            float p_xc = p_x * (std::sqrt(1 - p_y * p_y / 2 - p_z * p_z / 2 + p_y * p_y * p_z * p_z / 3));
            float p_yc = p_y * (std::sqrt(1 - p_x * p_x / 2 - p_z * p_z / 2 + p_x * p_x * p_z * p_z / 3));
            float p_zc = p_z * (std::sqrt(1 - p_y * p_y / 2 - p_x * p_x / 2 + p_y * p_y * p_x * p_x / 3));

            float z = fast_noise->GetSimplex(p_xc * 1000.f, p_yc * 1000.f, p_zc * 1000.f) * 0.9f;
            vertices.emplace_back(PlanetSectionVertex{glm::vec3{p_xc, p_yc, p_zc} * (mesh_scale + z), {x, y}, {0, 0, 1}});
        }
    }
    // Compute normals
    for (size_t x = 1; x < width - 1; ++x)
    {
        for (size_t y = 1; y < width - 1; ++y)
        {
            const glm::vec3& p = vertices[x + y * width].pos;

            glm::vec3 pa = vertices[x + 1 + y * width].pos - p;
            glm::vec3 pb = vertices[x + (y + 1) * width].pos - p;
            glm::vec3 pc = vertices[x - 1 + y * width].pos - p;
            glm::vec3 pd = vertices[x + (y - 1) * width].pos - p;

            glm::vec3 n1 = cross(normalize(pb), normalize(pa));
            glm::vec3 n2 = cross(normalize(pd), normalize(pc));

            vertices[x + y * width].norm = normalize((n1 + n2) / 2.f);
        }
    }

    // Generate index buffer
    for (size_t x = 0; x < width - 1; ++x)
    {
        for (size_t y = 0; y < width - 1; ++y)
        {
            indices.emplace_back(x + y * width);
            indices.emplace_back(x + 1 + (y + 1) * width);
            indices.emplace_back(x + 1 + y * width);

            indices.emplace_back(x + y * width);
            indices.emplace_back(x + (y + 1) * width);
            indices.emplace_back(x + 1 + (y + 1) * width);
        }
    }

    auto                 device = Eng::Engine::get().get_device().lock();
    Eng::Gfx::BufferData index_buffer(indices);
    mesh          = Eng::Gfx::Mesh::create("PlanetSection", device, Eng::Gfx::EBufferType::IMMUTABLE, Eng::Gfx::BufferData(vertices), &index_buffer);
    auto base_mat = Eng::Engine::get().asset_registry().create<Eng::MaterialAsset>("PlanetMaterial");
    base_mat->set_shader_code("planet_mat", std::vector{
                                  StageInputOutputDescription{0, 0, Eng::Gfx::ColorFormat::R32G32B32_SFLOAT},
                                  StageInputOutputDescription{1, 12, Eng::Gfx::ColorFormat::R32G32_SFLOAT},
                                  StageInputOutputDescription{2, 20, Eng::Gfx::ColorFormat::R32G32B32_SFLOAT},
                              });
    material = Eng::Engine::get().asset_registry().create<Eng::MaterialInstanceAsset>("PlanetMaterialInst", base_mat);
}

void PlanetComponent::draw(Eng::Gfx::CommandBuffer& command_buffer, const Eng::SceneView& view)
{
    material->set_scene_data(command_buffer.render_pass(), view.get_view_buffer());
    auto mat = material->get_base_resource(command_buffer.render_pass());
    command_buffer.bind_pipeline(mat);
    command_buffer.push_constant(Eng::Gfx::EShaderStage::Vertex, *mat, Eng::Gfx::BufferData(glm::identity<glm::mat4>()));
    command_buffer.bind_descriptors(*material->get_descriptor_resource(command_buffer.render_pass()), *mat);
    command_buffer.draw_mesh(*mesh);
}