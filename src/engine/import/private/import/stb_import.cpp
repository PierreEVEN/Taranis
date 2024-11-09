#include "import/stb_import.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "assets/asset_registry.hpp"
#include "assets/texture_asset.hpp"
#include "engine.hpp"
#include "object_ptr.hpp"
#include "gfx/vulkan/buffer.hpp"
#include "stb/stb_image.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace Eng
{
TObjectRef<TextureAsset> StbImporter::load_from_path(const std::filesystem::path& path)
{
    stbi_set_flip_vertically_on_load(true);  
    std::ifstream        input(path, std::ios::binary);
    std::vector<uint8_t> buffer(std::istreambuf_iterator(input), {});
    return load_raw(path.filename().string(), Gfx::BufferData(buffer.data(), 1, buffer.size()));
}

TObjectRef<TextureAsset> StbImporter::load_raw(const std::string& file_name, const Gfx::BufferData& raw)
{
    int      x      = 0, y = 0, channels = 0;
    stbi_uc* buffer = stbi_load_from_memory(static_cast<const stbi_uc*>(raw.data()), static_cast<int>(raw.get_byte_size()), &x, &y, &channels, 0);
    if (!buffer)
    {
        LOG_ERROR("Failed to load image {}", file_name);
        return {};
    }
    const auto text = Engine::get().asset_registry().create<TextureAsset>(file_name, Gfx::BufferData(buffer, 1, x * y * channels),
                                                                          TextureAsset::CreateInfos{.width = static_cast<uint32_t>(x), .height = static_cast<uint32_t>(y), .channels = static_cast<uint32_t>(channels)});
    stbi_image_free(buffer);
    return text;
}
} // namespace Eng