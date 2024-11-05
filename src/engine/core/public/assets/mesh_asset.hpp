#pragma once
#include "asset_base.hpp"

#include <memory>
#include <vector>
#include <glm/vec2.hpp>
#include<glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Engine
{
class BufferData;
class Mesh;

class MeshAsset : public AssetBase
{
public:
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec2 uv;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec4 color;
    };


    MeshAsset(const std::vector<Vertex>& vertices, const BufferData& indices);

    AssetType get_type() const override
    {
        return AssetType::Mesh;
    }

    const std::shared_ptr<Mesh>& get_mesh() const
    {
        return mesh;
    }

private:
    std::shared_ptr<Mesh> mesh;
};
}