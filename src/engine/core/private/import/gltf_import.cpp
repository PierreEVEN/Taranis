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
#include "assets/texture_asset.hpp"
#include "gfx/vulkan/buffer.hpp"

namespace Engine
{

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

    for (const auto& image : model.images)
    {
        LOG_INFO("load image...");
        Engine::get().asset_registry().create<TextureAsset>(path.filename().string(), BufferData(image.image.data(), 1, image.image.size()),
            TextureAsset::CreateInfos{.width = static_cast<uint32_t>(image.width), .height = static_cast<uint32_t>(image.height), .channels = static_cast<uint32_t>(image.component)});
    }
}
}