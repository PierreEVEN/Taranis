#pragma once
#include "asset_base.hpp"

#include <cstdint>
#include <memory>
#include "assets\texture_asset.gen.hpp"

namespace Engine
{
class BufferData;
class ImageView;
class Image;

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

    const std::shared_ptr<ImageView>& get_view() const
    {
        return view;
    }

    AssetType get_type() const override
    {
        return AssetType::Texture;
    }


private:
    friend class AssetRegistry;
    TextureAsset(const BufferData& data, CreateInfos create_infos);
    ~TextureAsset() = default;

    std::shared_ptr<Image>     image;
    std::shared_ptr<ImageView> view;

    uint32_t    width    = 0;
    uint32_t    height   = 0;
    uint32_t    channels = 0;
    CreateInfos infos;
};
}
