#pragma once
#include "asset_base.hpp"
#include "object_ptr.hpp"

#include "assets/mesh_asset.gen.hpp"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <vector>

namespace Engine
{
class MaterialInstanceAsset;
}

namespace Engine
{
class MaterialAsset;
}

namespace Engine
{
namespace Gfx
{
class Mesh;
class BufferData;
} // namespace Gfx

class MeshAsset : public AssetBase
{
    REFLECT_BODY()

public:
    struct Section
    {
        std::shared_ptr<Gfx::Mesh>        mesh;
        TObjectRef<MaterialInstanceAsset> material;
    };

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec2 uv;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec4 color;
    };

    MeshAsset()
    {
    }

    void add_section(const std::vector<Vertex>& vertices, const Gfx::BufferData& indices, const TObjectRef<MaterialInstanceAsset>& material);

    const std::vector<Section>& get_sections() const
    {
        return mesh_sections;
    }

private:
    std::vector<Section> mesh_sections;
};
} // namespace Engine