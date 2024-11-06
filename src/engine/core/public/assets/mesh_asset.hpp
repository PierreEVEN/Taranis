#pragma once
#include "asset_base.hpp"

#include <memory>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include "assets/mesh_asset.gen.hpp"

namespace Engine
{
namespace Gfx
{
class Mesh;
class BufferData;
}

class MeshAsset : public AssetBase
{
    REFLECT_BODY()

public:
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec2 uv;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec4 color;
    };


    MeshAsset(const std::vector<Vertex>& vertices, const Gfx::BufferData& indices);

    const std::shared_ptr<Gfx::Mesh>& get_mesh() const
    {
        return mesh;
    }

private:
    std::shared_ptr<Gfx::Mesh> mesh;
};
}