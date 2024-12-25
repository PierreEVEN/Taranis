#include "import/assimp_import.hpp"

#include "assets/asset_registry.hpp"
#include "assets/material_asset.hpp"
#include "assets/material_instance_asset.hpp"
#include "engine.hpp"
#include "gfx/vulkan/buffer.hpp"
#include "profiler.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <filesystem>

#include "assets/mesh_asset.hpp"
#include "assets/sampler_asset.hpp"
#include "assets/texture_asset.hpp"
#include "import/material_import.hpp"
#include "import/image_import.hpp"
#include "scene/components/mesh_component.hpp"
#include "scene/components/scene_component.hpp"

#include <numbers>

namespace Eng
{

AssimpImporter::AssimpImporter() : importer(std::make_shared<Assimp::Importer>())
{
}

AssimpImporter::SceneLoader::SceneLoader(const std::filesystem::path& in_file_path, const aiScene* in_scene, Scene& output_scene) : scene(in_scene), file_path(in_file_path)
{
    PROFILER_SCOPE(DecomposeAssimpScene);
    decompose_node(scene->mRootNode, {}, output_scene);
    scene->mRootNode;
}

Scene AssimpImporter::load_from_path(const std::filesystem::path& path) const
{
    Scene output_scene;
    PROFILER_SCOPE_NAMED(LoadAssimpSceneFromPath, std::format("Load assimp scene from path {}", path.filename().string()));
    const aiScene* scene = importer->ReadFile(path.string(), 0);
    if (!scene)
    {
        LOG_ERROR("Failed to load scene from path {}", path.string());
        return output_scene;
    }
    SceneLoader loader(path, scene, output_scene);
    return output_scene;
}

void AssimpImporter::SceneLoader::decompose_node(aiNode* node, TObjectRef<SceneComponent> parent, Scene& output_scene)
{
    TObjectRef<SceneComponent> this_component;
    if (node->mNumMeshes > 0)
    {
        auto new_mesh = Engine::get().asset_registry().create<MeshAsset>(node->mName.C_Str());
        for (size_t i = 0; i < node->mNumMeshes; ++i)
        {
            auto section = find_or_load_mesh(node->mMeshes[i]);
            if (!section)
                continue;
            new_mesh->add_section(section->name, section->vertices, *section->indices, section->mat);
        }
        if (parent)
        {
            this_component = parent->add_component<MeshComponent>(node->mName.C_Str(), new_mesh);
        }
        else
        {
            this_component = output_scene.add_component<MeshComponent>(node->mName.C_Str(), new_mesh);
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

TObjectRef<TextureAsset> AssimpImporter::SceneLoader::find_or_load_texture(const std::string& path)
{
    if (auto found = textures.find(path); found != textures.end())
        return found->second;

    PROFILER_SCOPE_NAMED(LoadTexture, std::format("Load texture {}", path));
    if (auto embed = scene->GetEmbeddedTexture(path.c_str()))
    {

        if (embed->mHeight == 0)
        {
            auto new_tex = ImageImport::load_raw(embed->mFilename.C_Str(), Gfx::BufferData(embed->pcData, 1, embed->mWidth));
            textures.emplace(path, new_tex);
            return new_tex;
        }
        else
        {
            assert(embed->achFormatHint == std::string("rgba8888"));
            auto new_mat = Engine::get().asset_registry().create<TextureAsset>(
                embed->mFilename.C_Str(), std::vector{Gfx::BufferData(embed->pcData, 1, embed->mWidth * embed->mHeight * 4)},
                TextureAsset::CreateInfos{.width = static_cast<uint32_t>(embed->mWidth), .height = static_cast<uint32_t>(embed->mHeight),
                                          .format = Gfx::ColorFormat::R8G8B8A8_UNORM});

            auto new_tex = ImageImport::load_raw(embed->mFilename.C_Str(), Gfx::BufferData(embed->pcData, 1, embed->mWidth));
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

        auto new_tex = ImageImport::load_from_path(fs_path);
        textures.emplace(path, new_tex);
        return new_tex;
    }
}


TObjectRef<TextureAsset> default_normal;
TObjectRef<TextureAsset> default_mrao;


TObjectRef<MaterialInstanceAsset> AssimpImporter::SceneLoader::find_or_load_material_instance(int id)
{
    if (auto found = materials.find(id); found != materials.end())
        return found->second;

    PROFILER_SCOPE_NAMED(LoadTexture, std::format("Load material instance {}", id));
    auto mat = scene->mMaterials[id];

    MaterialType type = MaterialType::Opaque_Albedo;

    if (mat->GetTextureCount(aiTextureType_NORMALS) > 0)
    {
        if (mat->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) > 0)
            type = MaterialType::Opaque_NormalMR;
        else
            type = MaterialType::Opaque_Normal;
    }
    else if (mat->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) > 0)
        type = MaterialType::Opaque_MR;

    auto new_mat = Engine::get().asset_registry().create<MaterialInstanceAsset>(std::string("MaterialInstance_") + mat->GetName().C_Str() + "_" + std::to_string(id), find_or_load_material(type));

    if (!default_normal)
    {
        std::vector<uint8_t> pixels = {
            0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0,
        };
        default_normal = Engine::get().asset_registry().create<TextureAsset>("DefaultNormal", std::vector{Gfx::BufferData(pixels.data(), 1, pixels.size())},
                                                                             TextureAsset::CreateInfos{.width = 2, .height = 2, .format = Gfx::ColorFormat::R8G8B8A8_UNORM});

    }
    if (!default_mrao)
    {
        std::vector<uint8_t> pixels = {
            0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0,
        };
        default_mrao = Engine::get().asset_registry().create<TextureAsset>("DefaultMrao", std::vector{Gfx::BufferData(pixels.data(), 1, pixels.size())},
                                                                           TextureAsset::CreateInfos{.width = 2, .height = 2, .format = Gfx::ColorFormat::R8G8B8A8_UNORM});
    }

    new_mat->set_sampler("sSampler", get_sampler());
    if (mat->GetTextureCount(aiTextureType_DIFFUSE) == 0)
        new_mat->set_permutation(new_mat->get_permutation().set("PARAM_ALBEDO_TEXTURE", false));
    if (mat->GetTextureCount(aiTextureType_NORMALS) == 0)
        new_mat->set_permutation(new_mat->get_permutation().set("PARAM_NORMAL_TEXTURE", false));
    if (mat->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) == 0)
        new_mat->set_permutation(new_mat->get_permutation().set("PARAM_METAL_ROUGHNESS_TEXTURE", false));

    for (uint32_t i = 0; i < mat->GetTextureCount(aiTextureType_DIFFUSE);)
    {
        aiString path;
        mat->GetTexture(aiTextureType_DIFFUSE, i, &path);
        if (auto diffuse = find_or_load_texture(path.C_Str()))
            new_mat->set_texture("albedo", diffuse);
        else
            new_mat->set_texture("albedo", TextureAsset::get_default_asset());
        break;
    }

    for (uint32_t i = 0; i < mat->GetTextureCount(aiTextureType_NORMALS);)
    {
        aiString path;
        mat->GetTexture(aiTextureType_NORMALS, i, &path);
        if (auto normal = find_or_load_texture(path.C_Str()))
            new_mat->set_texture("normal_map", normal);
        else
            new_mat->set_texture("normal_map", TextureAsset::get_default_asset());
        break;
    }

    for (uint32_t i = 0; i < mat->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS);)
    {
        aiString path;
        mat->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, i, &path);
        if (auto normal = find_or_load_texture(path.C_Str()))
            new_mat->set_texture("mr_map", normal);
        else
            new_mat->set_texture("mr_map", TextureAsset::get_default_asset());
        break;
    }

    new_mat->prepare_for_passes("gbuffers");
    new_mat->prepare_for_passes("shadows");

    return materials.emplace(id, new_mat).first->second;
}

TObjectRef<MaterialAsset> AssimpImporter::SceneLoader::find_or_load_material(MaterialType type)
{
    if (auto found = materials_base.find(type); found != materials_base.end())
        return found->second;

    PROFILER_SCOPE_NAMED(LoadTexture, std::format("Load material"));
    std::vector<std::string> features;

    switch (type)
    {
    case MaterialType::Opaque_Albedo:
        break;
    case MaterialType::Opaque_Normal:
        features.emplace_back("FEAT_NORMAL");
        break;
    case MaterialType::Opaque_NormalMR:
        features.emplace_back("FEAT_NORMAL");
        features.emplace_back("FEAT_MR");
        break;
    case MaterialType::Opaque_MR:
        features.emplace_back("FEAT_MR");
        break;
    case MaterialType::Translucent:
        break;
    }
    auto mat = Engine::get().asset_registry().create<MaterialAsset>("default_mesh");
    mat->set_shader_code("default_mesh", std::vector{
                             StageInputOutputDescription{0, 0, Gfx::ColorFormat::R32G32B32_SFLOAT},
                             StageInputOutputDescription{1, 12, Gfx::ColorFormat::R32G32_SFLOAT},
                             StageInputOutputDescription{2, 20, Gfx::ColorFormat::R32G32B32_SFLOAT},
                             StageInputOutputDescription{3, 32, Gfx::ColorFormat::R32G32B32_SFLOAT},
                             StageInputOutputDescription{4, 44, Gfx::ColorFormat::R32G32B32_SFLOAT},
                             StageInputOutputDescription{5, 56, Gfx::ColorFormat::R32G32B32A32_SFLOAT},
                         });

    return materials_base.emplace(type, mat).first->second;
}

std::shared_ptr<AssimpImporter::SceneLoader::MeshSection> AssimpImporter::SceneLoader::find_or_load_mesh(int id)
{
    if (auto found = meshes.find(id); found != meshes.end())
        return found->second;
    auto mesh = scene->mMeshes[id];

    PROFILER_SCOPE_NAMED(LoadTexture, std::format("Load mesh"));
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
        {
            vertices[i].tangent   = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            vertices[i].bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
        }
        if (mesh->HasVertexColors(0))
            vertices[i].color = glm::vec4(mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b, mesh->mColors[0][i].a);
    }

    if (mesh->mNumVertices <= UINT16_MAX)
    {
        uint32_t total_faces = mesh->mNumFaces;

        std::vector<uint16_t> triangles(total_faces * 3);

        for (uint32_t i = 0, initial_face = 0; i < total_faces; ++i, ++initial_face)
        {
            auto& face = mesh->mFaces[initial_face];
            if (face.mNumIndices == 3)
            {
                triangles[i * 3]     = static_cast<uint16_t>(face.mIndices[0]);
                triangles[i * 3 + 1] = static_cast<uint16_t>(face.mIndices[1]);
                triangles[i * 3 + 2] = static_cast<uint16_t>(face.mIndices[2]);
            }
            else if (face.mNumIndices == 4)
            {
                triangles[i * 3]     = static_cast<uint16_t>(face.mIndices[0]);
                triangles[i * 3 + 1] = static_cast<uint16_t>(face.mIndices[1]);
                triangles[i * 3 + 2] = static_cast<uint16_t>(face.mIndices[2]);
                ++total_faces;
                triangles.resize(total_faces * 3);
                ++i;
                triangles[i * 3]     = static_cast<uint16_t>(face.mIndices[0]);
                triangles[i * 3 + 1] = static_cast<uint16_t>(face.mIndices[2]);
                triangles[i * 3 + 2] = static_cast<uint16_t>(face.mIndices[3]);
            }
            else
            {
                LOG_ERROR("Unsupported mesh topology : {} ({})", mesh->mName.C_Str(), face.mNumIndices);
                meshes.emplace(id, nullptr);
                return nullptr;
            }
        }

        auto base_buffer = Gfx::BufferData(triangles.data(), 2, triangles.size());
        auto new_section = std::make_shared<MeshSection>(std::string(mesh->mName.C_Str()) + "_" + std::to_string(id), find_or_load_material_instance(mesh->mMaterialIndex), vertices, base_buffer.copy());
        meshes.emplace(id, new_section);
        return new_section;
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
        auto new_section = std::make_shared<MeshSection>(std::string(mesh->mName.C_Str()) + "_" + std::to_string(id), find_or_load_material_instance(mesh->mMaterialIndex), vertices,
                                                         Gfx::BufferData(triangles.data(), 4, triangles.size()).copy());
        meshes.emplace(id, new_section);
        return new_section;
    }
}

TObjectRef<SamplerAsset> AssimpImporter::SceneLoader::get_sampler()
{
    if (!sampler)
        sampler = Engine::get().asset_registry().create<SamplerAsset>("Sampler");
    return sampler;
}
} // namespace Eng