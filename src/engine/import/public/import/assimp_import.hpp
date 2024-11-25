#pragma once
#include "object_ptr.hpp"
#include "assets/mesh_asset.hpp"
#include "gfx/vulkan/buffer.hpp"

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
    enum class MaterialType
    {
        Opaque_Albedo,
        Opaque_Normal,
        Opaque_NormalMR,
        Opaque_MR,
        Translucent
    };

    AssimpImporter();

    struct SceneLoader
    {
        SceneLoader(const std::filesystem::path& in_file_path, const aiScene* in_scene, Scene& output_scene);

        void decompose_node(aiNode* node, TObjectRef<SceneComponent> parent, Scene& output_scene);

        struct MeshSection
        {
            TObjectRef<MaterialInstanceAsset> mat;
            std::vector<MeshAsset::Vertex>    vertices;
            std::shared_ptr<Gfx::BufferData>  indices;
        };

        TObjectRef<TextureAsset>          find_or_load_texture(const std::string& path);
        TObjectRef<MaterialInstanceAsset> find_or_load_material_instance(int id);
        TObjectRef<MaterialAsset>         find_or_load_material(MaterialType type);
        std::shared_ptr<MeshSection>      find_or_load_mesh(int id);
        TObjectRef<SamplerAsset>          get_sampler();

        std::unordered_map<std::string, TObjectRef<TextureAsset>>   textures;
        std::unordered_map<int, TObjectRef<MaterialInstanceAsset>>  materials;
        std::unordered_map<int, std::shared_ptr<MeshSection>>       meshes;
        TObjectRef<SamplerAsset>                                    sampler;
        const aiScene*                                              scene;
        std::unordered_map<MaterialType, TObjectRef<MaterialAsset>> materials_base;
        std::filesystem::path                                       file_path;
    };


    Scene load_from_path(const std::filesystem::path& path) const;

    std::shared_ptr<Assimp::Importer> importer;

};


}