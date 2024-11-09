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
        FreeImage_SetOutputMessage(
            [](FREE_IMAGE_FORMAT fif, const char* msg)
            {
                LOG_WARNING("FreeImage (format {}) : {}", static_cast<int>(fif), msg);
            });
        FreeImage_SetOutputMessageStdCall(
            [](FREE_IMAGE_FORMAT fif, const char* msg)
            {
                LOG_WARNING("FreeImage (format {}) : {}", static_cast<int>(fif), msg);
            });
    }
    ~FreeImageInitializer()
    {
        FreeImage_DeInitialise();
    }
};
static FreeImageInitializer _initializer;

TObjectRef<TextureAsset> StbImporter::load_from_path(const std::filesystem::path& path)
{
    FREE_IMAGE_FORMAT image_format = FreeImage_GetFileType(path.string().c_str(), 0);
    
    FIBITMAP*            image_raw = FreeImage_Load(image_format, path.string().c_str(), 0);


    
    FreeImage_Unload(image_raw);

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
    if (!image_raw)
    {
        LOG_ERROR("Failed to load image {} (format : {})", file_name, static_cast<int>(image_format));
        return {};
    }
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