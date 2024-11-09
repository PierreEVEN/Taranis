#include "import/stb_import.hpp"
#include "assets/asset_registry.hpp"
#include "assets/texture_asset.hpp"
#include "engine.hpp"
#include "object_ptr.hpp"
#include "gfx/vulkan/buffer.hpp"

#include <FreeImage.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace Eng
{

struct FreeImageInitializer
{
    FreeImageInitializer()
    {
        FreeImage_Initialise();
    }
    ~FreeImageInitializer()
    {
        FreeImage_DeInitialise();
    }
};
static FreeImageInitializer _initializer;


TObjectRef<TextureAsset> StbImporter::load_from_path(const std::filesystem::path& path)
{
    std::ifstream        input(path, std::ios::binary);
    std::vector<uint8_t> buffer(std::istreambuf_iterator(input), {});
    return load_raw(path.filename().string(), Gfx::BufferData(buffer.data(), 1, buffer.size()));
}

TObjectRef<TextureAsset> StbImporter::load_raw(const std::string& file_name, const Gfx::BufferData& raw)
{
    FIMEMORY*         mem_handle   = FreeImage_OpenMemory(const_cast<BYTE*>(static_cast<const BYTE*>(raw.data())), raw.get_byte_size());
    FREE_IMAGE_FORMAT image_format = FreeImage_GetFileTypeFromMemory(mem_handle, 0);
    if (image_format == FIF_UNKNOWN)
        image_format = FreeImage_GetFIFFromFilename(file_name.c_str());

    if (image_format == FIF_UNKNOWN)
    {
        LOG_ERROR("Failed to determine image format");
        return {};
    }

    FIBITMAP*         image_raw    = FreeImage_LoadFromMemory(image_format, mem_handle);
    FreeImage_CloseMemory(mem_handle);
    FIBITMAP* converted = nullptr;
    FreeImage_FlipVertical(converted);
    converted = FreeImage_ConvertTo32Bits(image_raw);
    FreeImage_Unload(image_raw);
    uint32_t x = FreeImage_GetWidth(converted);
    uint32_t y = FreeImage_GetHeight(converted);

    const auto text = Engine::get().asset_registry().create<TextureAsset>(file_name, Gfx::BufferData(FreeImage_GetBits(converted), 1, x * y * 4),
                                                                          TextureAsset::CreateInfos{.width = x, .height = y, .channels = 4});

    FreeImage_Unload(converted);
    return text;
}
} // namespace Eng