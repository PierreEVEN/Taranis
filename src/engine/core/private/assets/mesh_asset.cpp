#include "assets/mesh_asset.hpp"

#include "engine.hpp"
#include "gfx/mesh.hpp"

namespace Engine
{

MeshAsset::MeshAsset(const std::vector<Vertex>& vertices, const BufferData& indices)
{
    mesh = Mesh::create(get_name(), Engine::get().get_device(), EBufferType::IMMUTABLE, BufferData(vertices.data(), sizeof(Vertex), vertices.size()), &indices);
}
}