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

namespace Engine
{
class Scene;

class SamplerAsset;
class CameraComponent;
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
        SceneLoader(const std::filesystem::path& in_file_path, const aiScene* in_scene, Scene& output_scene, const TObjectRef<MaterialAsset>& in_base_material, const TObjectRef<CameraComponent>& in_temp_cam);

        void decompose_node(aiNode* node, TObjectRef<SceneComponent> parent, Scene& output_scene);

        struct MeshSection
        {
            TObjectRef<MaterialInstanceAsset> mat;
            std::vector<MeshAsset::Vertex>    vertices;
            std::shared_ptr<Gfx::BufferData>  indices;
        };

        TObjectRef<TextureAsset>          find_or_load_texture(std::string path);
        TObjectRef<MaterialInstanceAsset> find_or_load_material(int id);
        MeshSection&                      find_or_load_mesh(int id);
        TObjectRef<SamplerAsset>          get_sampler();

        std::unordered_map<std::string, TObjectRef<TextureAsset>>  textures;
        std::unordered_map<int, TObjectRef<MaterialInstanceAsset>> materials;
        std::unordered_map<int, std::shared_ptr<MeshSection>>      meshes;
        TObjectRef<SamplerAsset>                                   sampler;
        const aiScene*                                             scene;
        TObjectRef<MaterialAsset>                                  base_material;
        TObjectRef<CameraComponent>                                temp_cam;
        std::filesystem::path                                      file_path;
    };


    void load_from_path(const std::filesystem::path& path, Scene& output_scene, const TObjectRef<MaterialAsset>& in_base_material, const TObjectRef<CameraComponent>& in_temp_cam) const;

    std::shared_ptr<Assimp::Importer> importer;

};


}