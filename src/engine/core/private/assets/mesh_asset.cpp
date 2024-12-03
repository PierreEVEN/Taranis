#include "assets/mesh_asset.hpp"

#include "engine.hpp"
#include "gfx/mesh.hpp"

namespace Eng
{

void MeshAsset::add_section(const std::string& section_name, const std::vector<Vertex>& vertices, const Gfx::BufferData& indices, const TObjectRef<MaterialInstanceAsset>& material, const Bounds& in_bounds)
{
    Bounds section_bounds = in_bounds;
    if (!section_bounds)
    {
        for (const auto& vertex : vertices)
            section_bounds.add_point(vertex.pos);
    }
    bounds += section_bounds;

    mesh_sections.emplace_back(section_bounds, Gfx::Mesh::create(section_name, Engine::get().get_device(), Gfx::EBufferType::IMMUTABLE, Gfx::BufferData(vertices.data(), sizeof(Vertex), vertices.size()), &indices),
                               material);
}
} // namespace Eng