#include "import/image_import.hpp"
#include "assets/asset_registry.hpp"
#include "assets/texture_asset.hpp"
#include "engine.hpp"
#include "gfx/vulkan/buffer.hpp"
#include "object_ptr.hpp"
#include "profiler.hpp"
#include <vulkan/vulkan.h>
#include "dds_image/dds.hpp"

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

TObjectRef<TextureAsset> ImageImport::load_from_path(const std::filesystem::path& path)
{
    std::ifstream        input(path, std::ios::binary);
    std::vector<uint8_t> buffer(std::istreambuf_iterator(input), {});
    return load_raw(path.filename().string(), Gfx::BufferData(buffer.data(), 1, buffer.size()));
}

TObjectRef<TextureAsset> ImageImport::load_raw(const std::string& file_name, const Gfx::BufferData& raw)
{
    PROFILER_SCOPE_NAMED(LoadImage, std::format("Load image {}", file_name));

    if (std::filesystem::path(file_name).extension() == ".dds")
    {

        dds::Image image;

        if (readImage((uint8_t*)raw.data(), raw.get_byte_size(), &image) != dds::Success)
        {
            LOG_ERROR("Failed to read dds image {}", file_name);
            return {};
        }

        std::vector<Gfx::BufferData> mips;
        if (!image.data.empty())
            mips.emplace_back(Gfx::BufferData(image.data.data(), 1, image.data.size()));
        else
            for (const auto& mip : image.mipmaps)
                mips.emplace_back(mip.data(), 1, mip.size());

        const auto text = Engine::get().asset_registry().create<TextureAsset>(
            file_name, mips,
            TextureAsset::CreateInfos{
                .width = image.width,
                .height = image.height,
                .depth = image.depth,
                .format = static_cast<Gfx::ColorFormat>(dds::getVulkanFormat(image.format, image.supportsAlpha)),
                .array_size = image.arraySize
            });

        return text;
    }
    else
    {
        FIMEMORY*         mem_handle   = FreeImage_OpenMemory(const_cast<BYTE*>(static_cast<const BYTE*>(raw.data())), static_cast<DWORD>(raw.get_byte_size()));
        FREE_IMAGE_FORMAT image_format = FreeImage_GetFileTypeFromMemory(mem_handle, 0);
        if (image_format == FIF_UNKNOWN)
            image_format = FreeImage_GetFIFFromFilename(file_name.c_str());

        if (image_format == FIF_UNKNOWN)
        {
            LOG_ERROR("Failed to determine image format");
            return {};
        }

        FIBITMAP* image_raw = FreeImage_LoadFromMemory(image_format, mem_handle);
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

        const auto text = Engine::get().asset_registry().create<TextureAsset>(file_name, std::vector{Gfx::BufferData(FreeImage_GetBits(converted), 1, x * y * 4)},
                                                                              TextureAsset::CreateInfos{
                                                                                  .width = x,
                                                                                  .height = y,
                                                                                  .format = Gfx::ColorFormat::R8G8B8A8_UNORM
                                                                              });

        FreeImage_Unload(converted);
        return text;
    }
}
} // namespace Eng