#include "assets/mesh_asset.hpp"

#include "engine.hpp"
#include "gfx/mesh.hpp"

namespace Engine
{

void MeshAsset::add_section(const std::vector<Vertex>& vertices, const Gfx::BufferData& indices, const TObjectRef<MaterialInstanceAsset>& material)
{
    mesh_sections.emplace_back(Gfx::Mesh::create(get_name(), Engine::get().get_device(), Gfx::EBufferType::IMMUTABLE, Gfx::BufferData(vertices.data(), sizeof(Vertex), vertices.size()), &indices), material);
}
} // namespace Engine