#pragma once
#include "assets/mesh_asset.hpp"
#include "gfx/vulkan/buffer.hpp"
#include "object_ptr.hpp"

#include <memory>

struct aiTexture;

struct aiScene;
struct aiNode;

namespace Assimp
{
class Importer;
}

namespace std::filesystem
{
class path;
}

namespace Eng
{

struct MaterialMetaData
{
    bool two_sided = false;

    bool operator==(const MaterialMetaData& other) const
    {
        return other.two_sided == two_sided;
    }
};

} // namespace Eng

namespace std
{
template <class T> void hash_combine(::size_t& s, const T& v)
{
    ::std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

template <> struct hash<Eng::MaterialMetaData>
{
    size_t operator()(const Eng::MaterialMetaData& c) const noexcept
    {
        size_t result = 0;
        hash_combine(result, c.two_sided);
        return result;
    }
};
} // namespace std

namespace Eng {

class Scene;

class SamplerAsset;
class SceneComponent;
class MaterialAsset;
class TextureAsset;
class MaterialInstanceAsset;
class MeshAsset;

namespace Gfx
{
class BufferData;
}

class AssimpImporter
{
  public:

    AssimpImporter();

    struct SceneLoader
    {
        SceneLoader(const std::filesystem::path& in_file_path, const aiScene* in_scene, Scene& output_scene);

        void decompose_node(aiNode* node, TObjectRef<SceneComponent> parent, Scene& output_scene);

        struct MeshSection
        {
            std::string                       name;
            TObjectRef<MaterialInstanceAsset> mat;
            std::vector<MeshAsset::Vertex>    vertices;
            std::shared_ptr<Gfx::BufferData>  indices;
        };

        TObjectRef<TextureAsset>          find_or_load_texture(const std::string& path);
        TObjectRef<MaterialInstanceAsset> find_or_load_material_instance(int id);
        TObjectRef<MaterialAsset>         find_or_load_material(const MaterialMetaData& type);
        std::shared_ptr<MeshSection>      find_or_load_mesh(int id);
        TObjectRef<SamplerAsset>          get_sampler();

        ankerl::unordered_dense::map<std::string, TObjectRef<TextureAsset>>   textures;
        ankerl::unordered_dense::map<int, TObjectRef<MaterialInstanceAsset>>  materials;
        ankerl::unordered_dense::map<int, std::shared_ptr<MeshSection>>       meshes;
        TObjectRef<SamplerAsset>                                    sampler;
        const aiScene*                                              scene;
        ankerl::unordered_dense::map<MaterialMetaData, TObjectRef<MaterialAsset>> materials_base;
        std::filesystem::path                                       file_path;
    };

    Scene load_from_path(const std::filesystem::path& path) const;

    std::shared_ptr<Assimp::Importer> importer;
};

} // namespace Eng