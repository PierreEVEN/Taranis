#include "import/assimp_import.hpp"

#include "engine.hpp"
#include "assets/asset_registry.hpp"
#include "assets/material_instance_asset.hpp"
#include "gfx/vulkan/buffer.hpp"

#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include "assets/mesh_asset.hpp"
#include "assets/texture_asset.hpp"
#include "import/stb_import.hpp"
#include "scene/components/mesh_component.hpp"
#include "scene/components/scene_component.hpp"
#include "assets/sampler_asset.hpp"


namespace Eng
{


AssimpImporter::AssimpImporter() : importer(std::make_shared<Assimp::Importer>())
{
}

AssimpImporter::SceneLoader::SceneLoader(const std::filesystem::path&       in_file_path, const aiScene* in_scene, Scene& output_scene, const TObjectRef<MaterialAsset>& in_base_material,
                                         const TObjectRef<CameraComponent>& in_temp_cam)
    : scene(in_scene), base_material(in_base_material), temp_cam(in_temp_cam), file_path(in_file_path)
{

    decompose_node(scene->mRootNode, {}, output_scene);
    scene->mRootNode;

}

void AssimpImporter::load_from_path(const std::filesystem::path& path, Scene& output_scene, const TObjectRef<MaterialAsset>& in_base_material, const TObjectRef<CameraComponent>& in_temp_cam) const
{
    const aiScene* scene = importer->ReadFile(path.string(), 0);
    if (!scene)
    {
        LOG_ERROR("Failed to load scene from path {}", path.string());
        return;
    }
    SceneLoader loader(path, scene, output_scene, in_base_material, in_temp_cam);
}

void AssimpImporter::SceneLoader::decompose_node(aiNode* node, TObjectRef<SceneComponent> parent, Scene& output_scene)
{
    TObjectRef<SceneComponent> this_component;

    if (node->mNumMeshes > 0)
    {
        auto new_mesh = Engine::get().asset_registry().create<MeshAsset>(node->mName.C_Str());
        for (size_t i = 0; i < node->mNumMeshes; ++i)
        {
            auto& section = find_or_load_mesh(node->mMeshes[i]);
            new_mesh->add_section(section.vertices, *section.indices, section.mat);
        }
        if (parent)
        {
            this_component = parent->add_component<MeshComponent>(node->mName.C_Str(), temp_cam, new_mesh);
        }
        else
        {
            this_component = output_scene.add_component<MeshComponent>(node->mName.C_Str(), temp_cam, new_mesh);
        }
    }
    else
    {
        if (parent)
        {
            this_component = parent->add_component<SceneComponent>(node->mName.C_Str());
        }
        else
        {
            this_component = output_scene.add_component<SceneComponent>(node->mName.C_Str());
        }
    }

    for (size_t i = 0; i < node->mNumChildren; ++i)
        decompose_node(node->mChildren[i], this_component, output_scene);
}

TObjectRef<TextureAsset> AssimpImporter::SceneLoader::find_or_load_texture(std::string path)
{
    if (auto found = textures.find(path); found != textures.end())
        return found->second;

    if (auto embed = scene->GetEmbeddedTexture(path.c_str()))
    {

        if (embed->mHeight == 0)
        {
            auto new_tex = StbImporter::load_raw(embed->mFilename.C_Str(), Gfx::BufferData(embed->pcData, 1, embed->mWidth));
            textures.emplace(path, new_tex);
            return new_tex;
        }
        else
        {
            assert(embed->achFormatHint == std::string("rgba8888"));
            auto new_mat = Engine::get().asset_registry().create<TextureAsset>(embed->mFilename.C_Str(), Gfx::BufferData(embed->pcData, 1, embed->mWidth * embed->mHeight * 4),
                                                                               TextureAsset::CreateInfos{.width = static_cast<uint32_t>(embed->mWidth), .height = static_cast<uint32_t>(embed->mHeight), .channels = 4});

            auto new_tex = StbImporter::load_raw(embed->mFilename.C_Str(), Gfx::BufferData(embed->pcData, 1, embed->mWidth));
            textures.emplace(path, new_tex);
            return new_tex;
        }
    }
    else
    {
        std::filesystem::path fs_path(path);
        if (exists(fs_path));
        else if (exists(file_path.parent_path() / fs_path))
        {
            fs_path = file_path.parent_path() / fs_path;
        }
        else
        {
            LOG_FATAL("Failed to load texture {}", fs_path.string());
        }

        auto new_tex = StbImporter::load_from_path(fs_path);
        textures.emplace(path, new_tex);
        return new_tex;
    }
}

TObjectRef<MaterialInstanceAsset> AssimpImporter::SceneLoader::find_or_load_material(int id)
{
    if (auto found = materials.find(id); found != materials.end())
        return found->second;
    auto new_mat = Engine::get().asset_registry().create<MaterialInstanceAsset>("mat instance", base_material);

    auto mat = scene->mMaterials[id];

    for (uint32_t i = 0; i < mat->GetTextureCount(aiTextureType_DIFFUSE); ++i)
    {
        aiString path;
        mat->GetTexture(aiTextureType_DIFFUSE, i, &path);
        new_mat->set_sampler("sSampler", get_sampler());
        new_mat->set_texture("albedo", find_or_load_texture(path.C_Str()));
        break;
    }

    for (uint32_t i = 0; i < mat->GetTextureCount(aiTextureType_NORMALS); ++i)
    {
        aiString path;
        mat->GetTexture(aiTextureType_NORMALS, i, &path);

        new_mat->set_sampler("sSampler", get_sampler());
        new_mat->set_texture("normal", find_or_load_texture(path.C_Str()));
        break;
    }

    return materials.emplace(id, new_mat).first->second;
}

AssimpImporter::SceneLoader::MeshSection& AssimpImporter::SceneLoader::find_or_load_mesh(int id)
{
    if (auto found = meshes.find(id); found != meshes.end())
        return *found->second;
    auto mesh = scene->mMeshes[id];

    std::vector<MeshAsset::Vertex> vertices(mesh->mNumVertices, {});

    for (size_t i = 0; i < mesh->mNumVertices; ++i)
    {
        if (mesh->HasPositions())
            vertices[i].pos = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        if (mesh->HasTextureCoords(0))
            vertices[i].uv = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        if (mesh->HasNormals())
            vertices[i].normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        if (mesh->HasTangentsAndBitangents())
            vertices[i].tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
        if (mesh->HasVertexColors(0))
            vertices[i].color = glm::vec4(mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b, mesh->mColors[0][i].a);
    }

    if (mesh->mNumVertices <= UINT16_MAX)
    {
        std::vector<uint16_t> triangles(static_cast<size_t>(mesh->mNumFaces) * 3);

        for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
        {
            auto& face = mesh->mFaces[i];
            assert(face.mNumIndices == 3);
            triangles[i * 3]     = static_cast<uint16_t>(face.mIndices[0]);
            triangles[i * 3 + 1] = static_cast<uint16_t>(face.mIndices[1]);
            triangles[i * 3 + 2] = static_cast<uint16_t>(face.mIndices[2]);
        }

        auto base_buffer = Gfx::BufferData(triangles.data(), 2, triangles.size());
        auto new_section = std::make_shared<MeshSection>(find_or_load_material(mesh->mMaterialIndex), vertices, base_buffer.copy());
        meshes.emplace(id, new_section);
        return *new_section;
    }
    else
    {
        std::vector<uint32_t> triangles(static_cast<size_t>(mesh->mNumFaces) * 3);

        for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
        {
            auto& face = mesh->mFaces[i];
            assert(face.mNumIndices == 3);
            triangles[i * 3]     = face.mIndices[0];
            triangles[i * 3 + 1] = face.mIndices[1];
            triangles[i * 3 + 2] = face.mIndices[2];
        }
        auto new_section = std::make_shared<MeshSection>(find_or_load_material(mesh->mMaterialIndex), vertices, Gfx::BufferData(triangles.data(), 4, triangles.size()).copy());
        meshes.emplace(id, new_section);
        return *new_section;
    }
}

TObjectRef<SamplerAsset> AssimpImporter::SceneLoader::get_sampler()
{
    if (!sampler)
        sampler = Engine::get().asset_registry().create<SamplerAsset>("Sampler");
    return sampler;
}
}