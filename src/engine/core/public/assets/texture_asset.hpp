#pragma once
#include "asset_base.hpp"

#include <cstdint>
#include <memory>
#include "assets/texture_asset.gen.hpp"



namespace Engine
{
namespace Gfx
{
class BufferData;
class ImageView;
class Image;
}


class TextureAsset : public AssetBase
{
    REFLECT_BODY()
  public:
    struct CreateInfos
    {
        uint32_t width    = 0;
        uint32_t height   = 0;
        uint32_t channels = 0;
    };

    const std::shared_ptr<Gfx::ImageView>& get_view() const
    {
        return view;
    }

private:
    friend class AssetRegistry;
    TextureAsset(const Gfx::BufferData& data, CreateInfos create_infos);
    ~TextureAsset() = default;

    std::shared_ptr<Gfx::Image>     image;
    std::shared_ptr<Gfx::ImageView> view;

    uint32_t    width    = 0;
    uint32_t    height   = 0;
    uint32_t    channels = 0;
    CreateInfos infos;
};
}
