#pragma once
#include "asset_base.hpp"
#include "bounds.hpp"
#include "object_ptr.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <vector>

#include "assets/mesh_asset.gen.hpp"

namespace Eng
{
class MaterialInstanceAsset;
}

namespace Eng
{
class MaterialAsset;
}

namespace Eng
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
        Bounds                            bounds;
        std::shared_ptr<Gfx::Mesh>        mesh;
        TObjectRef<MaterialInstanceAsset> material;
    };

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec2 uv;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec3 bitangent;
        glm::vec4 color;
    };

    MeshAsset()
    {
    }

    void add_section(const std::string& section_name, const std::vector<Vertex>& vertices, const Gfx::BufferData& indices, const TObjectRef<MaterialInstanceAsset>& material, const Bounds& in_bounds = {});

    const std::vector<Section>& get_sections() const
    {
        return mesh_sections;
    }

    const Bounds& get_bounds() const
    {
        return bounds;
    }

private:
    Bounds               bounds;
    std::vector<Section> mesh_sections;
};
} // namespace Eng