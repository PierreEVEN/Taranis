#include "assets/mesh_asset.hpp"

#include "engine.hpp"
#include "gfx/mesh.hpp"

namespace Engine
{

MeshAsset::MeshAsset(const std::vector<Vertex>& vertices, const BufferData& indices)
{
    mesh = Mesh::create(get_name(), Engine::get().get_device(), sizeof(Vertex), EBufferType::IMMUTABLE);
    mesh->set_indexed_vertices(0, BufferData(vertices.data(), sizeof(Vertex), vertices.size()), 0, indices);
}
}