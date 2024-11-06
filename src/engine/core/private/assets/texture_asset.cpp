#include "assets/texture_asset.hpp"

#include "engine.hpp"
#include "gfx/vulkan/image.hpp"
#include "gfx/vulkan/image_view.hpp"


namespace Engine
{

TextureAsset::TextureAsset(const Gfx::BufferData& data, CreateInfos create_infos) : infos(create_infos)
{
    Gfx::ColorFormat format;
    std::vector<uint8_t> rgba_data;
    const uint8_t*             base_data;
    switch (infos.channels)
    {
    case 1:
        format = Gfx::ColorFormat::R8_UNORM;
        break;
    case 2:
        format = Gfx::ColorFormat::R8G8_UNORM;
        break;
    case 3:
        format = Gfx::ColorFormat::R8G8B8_UNORM;

        base_data = static_cast<const uint8_t*>(data.data());

        rgba_data.resize(static_cast<size_t>(create_infos.width) * static_cast<size_t>(create_infos.height) * 4);
        for (size_t i = 0; i < static_cast<size_t>(create_infos.width) * static_cast<size_t>(create_infos.height); ++i)
        {
            rgba_data[i * 4]     = base_data[i * 3];
            rgba_data[i * 4 + 1] = base_data[i * 3 + 1];
            rgba_data[i * 4 + 2] = base_data[i * 3 + 2];
            rgba_data[i * 4 + 3] = 255;
        }
        image =
            Gfx::Image::create(get_name(), Engine::get().get_device(), Gfx::ImageParameter{.format = Gfx::ColorFormat::R8G8B8A8_UNORM, .width = infos.width, .height = infos.height}, Gfx::BufferData(rgba_data.data(), 1, rgba_data.size()));
        break;
    case 4:
        format = Gfx::ColorFormat::R8G8B8A8_UNORM;
        break;
    default:
        LOG_FATAL("unsupported channel count : {}", infos.channels);
    }

    if (!image)
        image = Gfx::Image::create(get_name(), Engine::get().get_device(), Gfx::ImageParameter{.format = format, .width = infos.width, .height = infos.height}, data);
    view = Gfx::ImageView::create(get_name(), image);
}
}