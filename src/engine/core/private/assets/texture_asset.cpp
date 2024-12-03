#include "assets/texture_asset.hpp"

#include "assets/asset_registry.hpp"
#include "engine.hpp"
#include "gfx/vulkan/image.hpp"
#include "gfx/vulkan/image_view.hpp"

namespace Eng
{

const Gfx::ColorFormat& TextureAsset::get_format() const
{
    return image->get_params().format;
}

static TObjectRef<TextureAsset> default_asset = {};

TObjectRef<TextureAsset> TextureAsset::get_default_asset()
{
    if (!default_asset)
    {
        std::vector<uint8_t> pixels = {
            0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0,
        };
        default_asset = Engine::get().asset_registry().create<TextureAsset>("DefaultTexture", std::vector{Gfx::BufferData(pixels.data(), 1, pixels.size())},
                                                                CreateInfos{.width = 2, .height = 2, .format = Gfx::ColorFormat::R8G8B8A8_UNORM});
    }
    return default_asset;
}

TextureAsset::TextureAsset(const std::vector<Gfx::BufferData>& mips, const CreateInfos& create_infos) : infos(create_infos)
{
    switch (infos.format)
    {
    // rgb are not supported, add an alpha channel
    case Gfx::ColorFormat::R8G8B8_UNORM:
    {
        std::vector<Gfx::BufferData>      rgba_mips;
        std::vector<std::vector<uint8_t>> rgba_data_mips;

        for (const auto& data : mips)
        {
            const uint8_t* base_data = static_cast<const uint8_t*>(data.data());

            rgba_data_mips.emplace_back();
            auto& rgba_data = rgba_data_mips.back();

            rgba_data.resize(static_cast<size_t>(create_infos.width) * static_cast<size_t>(create_infos.height) * 4);
            for (size_t i = 0; i < static_cast<size_t>(create_infos.width) * static_cast<size_t>(create_infos.height); ++i)
            {
                rgba_data[i * 4]     = base_data[i * 3];
                rgba_data[i * 4 + 1] = base_data[i * 3 + 1];
                rgba_data[i * 4 + 2] = base_data[i * 3 + 2];
                rgba_data[i * 4 + 3] = 255;
            }
            rgba_mips.emplace_back(rgba_data.data(), 1, rgba_data.size());
        }
        image = Gfx::Image::create(get_name(), Engine::get().get_device(), Gfx::ImageParameter{
                                       .format = create_infos.format,
                                       .generate_mips = create_infos.generate_mips,
                                       .width = infos.width,
                                       .height = infos.height,
                                       .depth = infos.depth,
                                       .array_size = infos.array_size
                                   },
                                   rgba_mips);
        break;
    }
    }

    if (!image)
        image = Gfx::Image::create(get_name(), Engine::get().get_device(), Gfx::ImageParameter{
                                       .format = infos.format,
                                       .generate_mips = create_infos.generate_mips,
                                       .width = infos.width,
                                       .height = infos.height,
                                       .depth = infos.depth,
                                       .array_size = infos.array_size
                                   }, mips);
    view = Gfx::ImageView::create(get_name(), image);
}
} // namespace Eng