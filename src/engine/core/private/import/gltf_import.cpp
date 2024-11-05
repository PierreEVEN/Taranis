#include "import/gltf_import.hpp"
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#define TINYGLTF_USE_CPP14
#define TINYGLTF_NOEXCEPTION
#include "engine.hpp"
#include "logger.hpp"
#include "tiny_gltf.h"
#include "assets/asset_registry.hpp"
#include "assets/mesh_asset.hpp"
#include "assets/texture_asset.hpp"
#include "gfx/vulkan/buffer.hpp"

namespace Engine
{

template <typename In> void load_positions(std::vector<MeshAsset::Vertex>& vertices, In* data)
{
    

}




void GltfImporter::load_from_path(const std::filesystem::path& path)
{
    tinygltf::TinyGLTF loader;

    tinygltf::Model model;
    std::string     warn;
    std::string     err;

    //loader.LoadBinaryFromFile()
    if (!loader.LoadASCIIFromFile(&model, &err, &warn, path.string()))
        LOG_ERROR("Failed to load gltf : {}", err);
    if (!warn.empty())
        LOG_WARNING("{}", warn);

    std::unordered_map<uint32_t, BufferData> buffer_views;
    for (size_t i = 0; i < model.bufferViews.size(); ++i)
    {
        const tinygltf::BufferView& bufferView = model.bufferViews[i];
        if (bufferView.target == 0) // TODO impl drawarrays
            LOG_ERROR("WARN: bufferView.target is zero");

        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
        buffer_views[i]                = BufferData(buffer.data.data(), bufferView.byteStride, buffer.data.size());
    }

    for (const auto& mesh : model.meshes)
    {
        LOG_INFO("load mesh...");
        for (const auto& primitive : mesh.primitives)
        {
            std::vector<MeshAsset::Vertex> vertices;

            const tinygltf::Accessor& index_accessor = model.accessors[primitive.indices];

            if (auto positions = primitive.attributes.find("POSITION"); positions != primitive.attributes.end())
            {
                const tinygltf::Accessor& accessor    = model.accessors[positions->second];
                const auto&               buffer_view = model.bufferViews[accessor.bufferView];
                int                       byteStride  = accessor.ByteStride(model.bufferViews[accessor.bufferView]);
                
                if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
                    LOG_FATAL("Invalid position size : {}", accessor.componentType);
                switch (accessor.componentType)
                {
                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                    switch (accessor.type)
                    {
                    case TINYGLTF_TYPE_SCALAR:
                    {
                        float* data = reinterpret_cast<float*>(accessor.byteOffset);
                    }
                    break;
                    case TINYGLTF_TYPE_VEC2:
                    {
                        glm::vec2* data = reinterpret_cast<glm::vec2*>(accessor.byteOffset);
                    }
                    break;
                    case TINYGLTF_TYPE_VEC3:
                    {
                        glm::vec3* data = reinterpret_cast<glm::vec3*>(accessor.byteOffset);
                    }
                    break;
                    default:
                        LOG_FATAL("Invalid accessor type : {}", accessor.type);
                    }
                default:
                    LOG_FATAL("Invalid component size : {}", accessor.componentType);
                }
            }
        }
    }

    for (const auto& image : model.images)
    {
        LOG_INFO("load image...");
        Engine::get().asset_registry().create<TextureAsset>(path.filename().string(), BufferData(image.image.data(), 1, image.image.size()),
                                                            TextureAsset::CreateInfos{.width = static_cast<uint32_t>(image.width), .height = static_cast<uint32_t>(image.height),
                                                                                      .channels = static_cast<uint32_t>(image.component)});
    }
}
}