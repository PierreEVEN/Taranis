#include "import/gltf_import.hpp"
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#define TINYGLTF_USE_CPP14
#define TINYGLTF_NOEXCEPTION
#include "assets/asset_registry.hpp"
#include "assets/mesh_asset.hpp"
#include "assets/texture_asset.hpp"
#include "engine.hpp"
#include "gfx/vulkan/buffer.hpp"
#include "logger.hpp"
#include "tiny_gltf.h"

namespace Engine
{

void GltfImporter::load_from_path(const std::filesystem::path& path)
{
    tinygltf::TinyGLTF loader;

    tinygltf::Model model;
    std::string     warn;
    std::string     err;

    // loader.LoadBinaryFromFile()
    if (!loader.LoadASCIIFromFile(&model, &err, &warn, path.string()))
        LOG_ERROR("Failed to load gltf : {}", err);
    if (!warn.empty())
        LOG_WARNING("{}", warn);

    std::unordered_map<uint32_t, Gfx::BufferData> buffer_views;
    for (size_t i = 0; i < model.bufferViews.size(); ++i)
    {
        const tinygltf::BufferView& bufferView = model.bufferViews[i];
        if (bufferView.target == 0) // TODO impl draw arrays
            LOG_ERROR("WARN: bufferView.target is zero");

        const tinygltf::Buffer& buffer         = model.buffers[bufferView.buffer];
        buffer_views[static_cast<uint32_t>(i)] = Gfx::BufferData(buffer.data.data(), bufferView.byteStride, buffer.data.size());
    }

    for (const auto& mesh : model.meshes)
    {
        int id = 0;
        for (const auto& primitive : mesh.primitives)
        {
            id++;
            LOG_INFO("load primitive {}", mesh.name + "_#" + std::to_string(id));
            std::vector<MeshAsset::Vertex> vertices;

            const tinygltf::Accessor& index_accessor = model.accessors[primitive.indices];

            size_t componentSizeInBytes = tinygltf::GetComponentSizeInBytes(static_cast<uint32_t>(index_accessor.componentType));
            size_t numComponents        = tinygltf::GetNumComponentsInType(static_cast<uint32_t>(index_accessor.type));

            if (numComponents <= 0 || componentSizeInBytes <= 0)
                LOG_FATAL("Failed to deduce index buffer type");

            const auto&     indices_view = model.bufferViews[index_accessor.bufferView];
            Gfx::BufferData indices(&model.buffers[indices_view.buffer].data.at(indices_view.byteOffset + index_accessor.byteOffset), componentSizeInBytes * numComponents, index_accessor.count);

            if (auto positions = primitive.attributes.find("POSITION"); positions != primitive.attributes.end())
            {
                const tinygltf::Accessor& accessor = model.accessors[positions->second];
                vertices.resize(accessor.count, MeshAsset::Vertex{});
                assert(accessor.type == TINYGLTF_TYPE_VEC3 && accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                const auto& view = model.bufferViews[accessor.bufferView];
                glm::vec3*  bfr  = reinterpret_cast<glm::vec3*>(&model.buffers[view.buffer].data.at(view.byteOffset + accessor.byteOffset));
                for (size_t i = 0; i < accessor.count; ++i)
                    vertices[i].pos = bfr[i];
            }
            if (auto texcoords = primitive.attributes.find("TEXCOORD_0"); texcoords != primitive.attributes.end())
            {
                const tinygltf::Accessor& accessor = model.accessors[texcoords->second];
                vertices.resize(accessor.count, MeshAsset::Vertex{});
                assert(accessor.type == TINYGLTF_TYPE_VEC2 && accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                const auto& view = model.bufferViews[accessor.bufferView];
                glm::vec2*  bfr  = reinterpret_cast<glm::vec2*>(&model.buffers[view.buffer].data.at(view.byteOffset + accessor.byteOffset));
                for (size_t i = 0; i < accessor.count; ++i)
                    vertices[i].uv = bfr[i];
            }
            if (auto normals = primitive.attributes.find("NORMAL"); normals != primitive.attributes.end())
            {
                const tinygltf::Accessor& accessor = model.accessors[normals->second];
                vertices.resize(accessor.count, MeshAsset::Vertex{});
                assert(accessor.type == TINYGLTF_TYPE_VEC3 && accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                const auto& view = model.bufferViews[accessor.bufferView];
                glm::vec3*  bfr  = reinterpret_cast<glm::vec3*>(&model.buffers[view.buffer].data.at(view.byteOffset + accessor.byteOffset));
                for (size_t i = 0; i < accessor.count; ++i)
                    vertices[i].normal = bfr[i];
            }
            if (auto tangents = primitive.attributes.find("TANGENT"); tangents != primitive.attributes.end())
            {
                const tinygltf::Accessor& accessor = model.accessors[tangents->second];
                vertices.resize(accessor.count, MeshAsset::Vertex{});
                assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                if (accessor.type == TINYGLTF_TYPE_VEC3)
                {
                    const auto& view = model.bufferViews[accessor.bufferView];
                    glm::vec3*  bfr  = reinterpret_cast<glm::vec3*>(&model.buffers[view.buffer].data.at(view.byteOffset + accessor.byteOffset));
                    for (size_t i = 0; i < accessor.count; ++i)
                        vertices[i].tangent = bfr[i];
                }
                else if (accessor.type == TINYGLTF_TYPE_VEC4)
                {
                    const auto& view = model.bufferViews[accessor.bufferView];
                    glm::vec4*  bfr  = reinterpret_cast<glm::vec4*>(&model.buffers[view.buffer].data.at(view.byteOffset + accessor.byteOffset));
                    for (size_t i = 0; i < accessor.count; ++i)
                        vertices[i].tangent = bfr[i];
                }
                else
                    LOG_FATAL("Unsupported gltf type : {}", accessor.type);
            }
            if (auto colors = primitive.attributes.find("COLOR_0"); colors != primitive.attributes.end())
            {
                const tinygltf::Accessor& accessor = model.accessors[colors->second];
                vertices.resize(accessor.count, MeshAsset::Vertex{});
                assert(accessor.type == TINYGLTF_TYPE_VEC4 && accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                const auto& view = model.bufferViews[accessor.bufferView];
                glm::vec4*  bfr  = reinterpret_cast<glm::vec4*>(&model.buffers[view.buffer].data.at(view.byteOffset + accessor.byteOffset));
                for (size_t i = 0; i < accessor.count; ++i)
                    vertices[i].color = bfr[i];
            }

            Engine::get().asset_registry().create<MeshAsset>(path.filename().string() + mesh.name + "_#" + std::to_string(id), vertices, indices);
        }
    }

    for (const auto& image : model.images)
    {
        LOG_INFO("load image {}", image.name);
        Engine::get().asset_registry().create<TextureAsset>(
            path.filename().string() + "_" + image.name, Gfx::BufferData(image.image.data(), 1, image.image.size()),
            TextureAsset::CreateInfos{.width = static_cast<uint32_t>(image.width), .height = static_cast<uint32_t>(image.height), .channels = static_cast<uint32_t>(image.component)});
    }
}
} // namespace Engine