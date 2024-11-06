#include "assets/mesh_asset.hpp"

#include "engine.hpp"
#include "gfx/mesh.hpp"

namespace Engine
{

MeshAsset::MeshAsset(const std::vector<Vertex>& vertices, const Gfx::BufferData& indices)
{
    mesh = Gfx::Mesh::create(get_name(), Engine::get().get_device(), Gfx::EBufferType::IMMUTABLE, Gfx::BufferData(vertices.data(), sizeof(Vertex), vertices.size()), &indices);
}
}